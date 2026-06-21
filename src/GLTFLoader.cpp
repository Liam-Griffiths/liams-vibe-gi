// GLTFLoader.cpp - glTF 2.0 import via cgltf, mapped onto Mesh + Material/Texture.
#include "../include/GLTFLoader.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <unordered_map>
#include <iostream>
#include <string>
#include <cstring>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

// stb_image is implemented in Material.cpp; here we only need the declarations to decode
// the packed metallic-roughness image so it can be split into separate R8 maps.
#include "stb_image.h"

namespace {

std::string baseDirOf(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? std::string("") : path.substr(0, slash + 1);
}

// Resolve a (possibly percent-encoded) glTF image URI to a filesystem path.
std::string resolveImagePath(const std::string& baseDir, const char* uri) {
    if (!uri) return "";
    std::string decoded(uri);
    cgltf_decode_uri(&decoded[0]);
    decoded.resize(std::strlen(decoded.c_str())); // drop any tail left by in-place decode
    return baseDir + decoded;
}

// Load (and cache) a glTF image as a normal RGB(A) texture by file path.
Texture* getFileTexture(cgltf_texture_view& view, const std::string& baseDir,
                        std::unordered_map<cgltf_image*, Texture*>& cache, GLTFModel& model) {
    if (!view.texture || !view.texture->image) return nullptr;
    cgltf_image* img = view.texture->image;
    auto it = cache.find(img);
    if (it != cache.end()) return it->second;
    if (!img->uri) return nullptr; // embedded (glb) images unsupported in this minimal loader
    std::string path = resolveImagePath(baseDir, img->uri);
    auto tex = std::make_unique<Texture>();
    if (!tex->loadFromFile(path)) { cache[img] = nullptr; return nullptr; }
    Texture* raw = tex.get();
    model.textures.push_back(std::move(tex));
    cache[img] = raw;
    return raw;
}

// glTF packs roughness in G and metallic in B of one image, but the engine samples
// roughnessMap/metallicMap on R. Decode the image once and upload two single-channel maps.
void loadMetallicRoughness(cgltf_texture_view& view, const std::string& baseDir,
                           GLTFModel& model, Texture*& roughOut, Texture*& metalOut) {
    roughOut = nullptr; metalOut = nullptr;
    if (!view.texture || !view.texture->image || !view.texture->image->uri) return;
    std::string path = resolveImagePath(baseDir, view.texture->image->uri);
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 4);
    if (!px) { std::cerr << "glTF: failed to load metallic-roughness " << path << std::endl; return; }

    std::vector<unsigned char> rough(static_cast<size_t>(w) * h);
    std::vector<unsigned char> metal(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
        rough[i] = px[i * 4 + 1]; // G
        metal[i] = px[i * 4 + 2]; // B
    }
    stbi_image_free(px);

    auto rt = std::make_unique<Texture>();
    if (rt->loadFromMemory(rough.data(), w, h, 1)) { roughOut = rt.get(); model.textures.push_back(std::move(rt)); }
    auto mt = std::make_unique<Texture>();
    if (mt->loadFromMemory(metal.data(), w, h, 1)) { metalOut = mt.get(); model.textures.push_back(std::move(mt)); }
}

Material* buildMaterial(cgltf_material* gmat, const std::string& baseDir, GLTFModel& model,
                        std::unordered_map<cgltf_image*, Texture*>& cache) {
    auto mat = std::make_unique<Material>();
    mat->ownsTextures = false; // textures owned by GLTFModel::textures

    if (gmat && gmat->has_pbr_metallic_roughness) {
        cgltf_pbr_metallic_roughness& pbr = gmat->pbr_metallic_roughness;
        mat->baseColor = glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
        mat->roughness = pbr.roughness_factor;
        mat->metallic = pbr.metallic_factor;
        mat->albedoMap = getFileTexture(pbr.base_color_texture, baseDir, cache, model);
        loadMetallicRoughness(pbr.metallic_roughness_texture, baseDir, model, mat->roughnessMap, mat->metallicMap);
        mat->opacity = pbr.base_color_factor[3]; // alpha; meaningful for alphaMode BLEND
    }
    if (gmat) {
        // Transparency: alphaMode BLEND uses base-color alpha; KHR_materials_transmission
        // (+ KHR_materials_ior) describe refractive glass. Both route to the forward pass.
        if (gmat->alpha_mode != cgltf_alpha_mode_blend) mat->opacity = 1.0f; // opaque/mask are not blended
        if (gmat->has_transmission) mat->transmission = gmat->transmission.transmission_factor;
        if (gmat->has_ior) mat->ior = gmat->ior.ior;

        mat->normalMap = getFileTexture(gmat->normal_texture, baseDir, cache, model);
        mat->aoMap = getFileTexture(gmat->occlusion_texture, baseDir, cache, model);
        mat->emissionMap = getFileTexture(gmat->emissive_texture, baseDir, cache, model);
        float strength = gmat->has_emissive_strength ? gmat->emissive_strength.emissive_strength : 1.0f;
        mat->emission = glm::vec3(gmat->emissive_factor[0], gmat->emissive_factor[1], gmat->emissive_factor[2]) * strength;
    } else {
        mat->baseColor = glm::vec3(0.7f); // default gray
    }

    Material* raw = mat.get();
    model.materials.push_back(std::move(mat));
    return raw;
}

const cgltf_accessor* findAttr(const cgltf_primitive& prim, cgltf_attribute_type type, int index = 0) {
    for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
        if (prim.attributes[i].type == type && prim.attributes[i].index == index)
            return prim.attributes[i].data;
    }
    return nullptr;
}

void buildPrimitive(const cgltf_primitive& prim, const glm::mat4& world,
                    Material* material, GLTFModel& model) {
    if (prim.type != cgltf_primitive_type_triangles) return;
    const cgltf_accessor* posA = findAttr(prim, cgltf_attribute_type_position);
    if (!posA) return;
    const cgltf_accessor* nrmA = findAttr(prim, cgltf_attribute_type_normal);
    const cgltf_accessor* uvA  = findAttr(prim, cgltf_attribute_type_texcoord, 0);
    const cgltf_accessor* tanA = findAttr(prim, cgltf_attribute_type_tangent);

    glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(world));

    cgltf_size vcount = posA->count;
    std::vector<glm::vec3> P(vcount), N(vcount), T(vcount);
    std::vector<glm::vec2> UV(vcount);
    std::vector<float> Tw(vcount, 1.0f);
    for (cgltf_size i = 0; i < vcount; ++i) {
        float v[4] = {0,0,0,0};
        cgltf_accessor_read_float(posA, i, v, 3);
        P[i] = glm::vec3(world * glm::vec4(v[0], v[1], v[2], 1.0f));
        if (nrmA) { cgltf_accessor_read_float(nrmA, i, v, 3); N[i] = glm::normalize(normalMat * glm::vec3(v[0], v[1], v[2])); }
        if (uvA)  { cgltf_accessor_read_float(uvA, i, v, 2);  UV[i] = glm::vec2(v[0], 1.0f - v[1]); } // flip V for GL
        if (tanA) { cgltf_accessor_read_float(tanA, i, v, 4); T[i] = glm::normalize(normalMat * glm::vec3(v[0], v[1], v[2])); Tw[i] = v[3]; }
    }

    // Build a flat (de-indexed) triangle vertex list - matches the engine's Mesh format.
    std::vector<unsigned int> idx;
    if (prim.indices) {
        idx.resize(prim.indices->count);
        for (cgltf_size i = 0; i < prim.indices->count; ++i) idx[i] = (unsigned int)cgltf_accessor_read_index(prim.indices, i);
    } else {
        idx.resize(vcount);
        for (cgltf_size i = 0; i < vcount; ++i) idx[i] = (unsigned int)i;
    }

    std::vector<Vertex> verts;
    verts.reserve(idx.size());
    for (size_t t = 0; t + 2 < idx.size(); t += 3) {
        unsigned int i0 = idx[t], i1 = idx[t + 1], i2 = idx[t + 2];
        glm::vec3 faceN = glm::normalize(glm::cross(P[i1] - P[i0], P[i2] - P[i0]));
        // Fallback tangent from positions/UVs when the primitive has none.
        glm::vec3 faceT(1, 0, 0);
        if (!tanA) {
            glm::vec2 duv1 = UV[i1] - UV[i0], duv2 = UV[i2] - UV[i0];
            float d = duv1.x * duv2.y - duv2.x * duv1.y;
            if (fabs(d) > 1e-8f) {
                float r = 1.0f / d;
                faceT = glm::normalize((P[i1] - P[i0]) * duv2.y - (P[i2] - P[i0]) * duv1.y) * r;
                if (glm::any(glm::isnan(faceT))) faceT = glm::vec3(1, 0, 0);
            }
        }
        unsigned int tri[3] = {i0, i1, i2};
        for (int k = 0; k < 3; ++k) {
            unsigned int vi = tri[k];
            Vertex vx;
            vx.Position = P[vi];
            vx.Normal = nrmA ? N[vi] : faceN;
            vx.TexCoords = uvA ? UV[vi] : glm::vec2(0.0f);
            glm::vec3 tang = tanA ? T[vi] : faceT;
            vx.Tangent = tang;
            vx.Bitangent = glm::cross(vx.Normal, tang) * (tanA ? Tw[vi] : 1.0f);
            verts.push_back(vx);

            model.boundsMin = glm::min(model.boundsMin, vx.Position);
            model.boundsMax = glm::max(model.boundsMax, vx.Position);
        }
    }
    if (verts.empty()) return;

    auto mesh = std::make_unique<Mesh>(std::move(verts));
    Mesh* raw = mesh.get();
    model.meshes.push_back(std::move(mesh));
    model.instances.push_back({raw, material});
}

} // namespace

GLTFModel loadGLTF(const std::string& path) {
    GLTFModel model;
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        std::cerr << "glTF: failed to parse " << path << std::endl;
        return model;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        std::cerr << "glTF: failed to load buffers for " << path << std::endl;
        cgltf_free(data);
        return model;
    }

    std::string baseDir = baseDirOf(path);
    std::unordered_map<cgltf_image*, Texture*> texCache;
    std::unordered_map<cgltf_material*, Material*> matCache;

    // Initialise bounds from a large box so min/max accumulate correctly.
    model.boundsMin = glm::vec3(1e30f);
    model.boundsMax = glm::vec3(-1e30f);

    Material* defaultMat = nullptr;

    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        cgltf_node* node = &data->nodes[n];
        if (!node->mesh) continue;
        float wm[16];
        cgltf_node_transform_world(node, wm);
        glm::mat4 world = glm::make_mat4(wm);

        for (cgltf_size p = 0; p < node->mesh->primitives_count; ++p) {
            cgltf_primitive& prim = node->mesh->primitives[p];
            Material* mat = nullptr;
            if (prim.material) {
                auto it = matCache.find(prim.material);
                mat = (it != matCache.end()) ? it->second
                                             : (matCache[prim.material] = buildMaterial(prim.material, baseDir, model, texCache));
            } else {
                if (!defaultMat) defaultMat = buildMaterial(nullptr, baseDir, model, texCache);
                mat = defaultMat;
            }
            buildPrimitive(prim, world, mat, model);
        }
    }

    cgltf_free(data);

    if (model.instances.empty()) {
        std::cerr << "glTF: no triangle geometry found in " << path << std::endl;
        return model;
    }
    model.loaded = true;
    std::cout << "glTF: loaded " << path << " - " << model.instances.size() << " primitives, "
              << model.materials.size() << " materials, " << model.textures.size() << " textures" << std::endl;
    return model;
}
