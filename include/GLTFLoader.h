// GLTFLoader.h - minimal glTF 2.0 importer (geometry + metallic-roughness PBR)
// Maps glTF onto the engine's existing Mesh + Material/Texture types so loaded scenes
// render through the unchanged g-buffer path.
#ifndef GLTFLOADER_H
#define GLTFLOADER_H

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Material.h"

// One drawable: a mesh with its (non-owning) material and a baked world transform.
// Node transforms are baked into the mesh vertices, so modelMatrix is identity here and
// kept only for completeness.
struct GLTFInstance {
    Mesh* mesh;
    Material* material;
};

// Owns everything loaded from a glTF file. Keep this alive for the lifetime of the scene.
struct GLTFModel {
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Texture>> textures;   // image cache (shared by materials)
    std::vector<std::unique_ptr<Material>> materials; // one per glTF material (+ default)
    std::vector<GLTFInstance> instances;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    bool loaded = false;
};

// Loads a .gltf (external buffers/textures) file. Returns loaded=false on failure.
GLTFModel loadGLTF(const std::string& path);

#endif // GLTFLOADER_H
