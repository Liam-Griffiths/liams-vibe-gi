#include "../include/Material.h"
#include <iostream>
#include <OpenGL/gl3.h>

// You might need to install stb_image, for now I'll add a simple version
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Texture implementation
Texture::Texture() : id(0), width(0), height(0), nrChannels(0) {}

Texture::Texture(const std::string& path, const std::string& type) 
    : type(type), path(path), width(0), height(0), nrChannels(0) {
    loadFromFile(path);
}

Texture::~Texture() {
    if (id != 0) {
        glDeleteTextures(1, &id);
    }
}

bool Texture::loadFromFile(const std::string& path) {
    this->path = path;
    
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Load image
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << nrChannels << " channels)" << std::endl;
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return false;
    }
    
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Material implementation
Material::Material() {
    initializeDefaults();
}

Material::Material(const glm::vec3& color, float roughness, float metallic) 
    : baseColor(color), roughness(roughness), metallic(metallic) {
    initializeDefaults();
}

Material::~Material() {
    delete albedoMap;
    delete normalMap;
    delete roughnessMap;
    delete metallicMap;
    delete aoMap;
    delete heightMap;
}

void Material::initializeDefaults() {
    baseColor = glm::vec3(1.0f);
    roughness = 0.5f;
    metallic = 0.0f;
    ambientOcclusion = 1.0f;
    
    albedoMap = nullptr;
    normalMap = nullptr;
    roughnessMap = nullptr;
    metallicMap = nullptr;
    aoMap = nullptr;
    heightMap = nullptr;
}

bool Material::loadPBRMaterial(const std::string& baseName) {
    bool success = true;
    
    // Try to load albedo/base color
    std::string albedoPath = "textures/" + baseName + "_basecolor.jpg";
    albedoMap = new Texture();
    if (!albedoMap->loadFromFile(albedoPath)) {
        // Try alternative naming
        albedoPath = "textures/" + baseName + "_albedo.jpg";
        if (!albedoMap->loadFromFile(albedoPath)) {
            delete albedoMap;
            albedoMap = nullptr;
            std::cout << "Warning: Could not load albedo map for " << baseName << std::endl;
        }
    }
    
    // Try to load normal map
    std::string normalPath = "textures/" + baseName + "_normal.jpg";
    normalMap = new Texture();
    if (!normalMap->loadFromFile(normalPath)) {
        delete normalMap;
        normalMap = nullptr;
        std::cout << "Warning: Could not load normal map for " << baseName << std::endl;
    }
    
    // Try to load roughness map
    std::string roughnessPath = "textures/" + baseName + "_roughness.jpg";
    roughnessMap = new Texture();
    if (!roughnessMap->loadFromFile(roughnessPath)) {
        delete roughnessMap;
        roughnessMap = nullptr;
        std::cout << "Warning: Could not load roughness map for " << baseName << std::endl;
    }
    
    // Try to load ambient occlusion map
    std::string aoPath = "textures/" + baseName + "_ambientOcclusion.jpg";
    aoMap = new Texture();
    if (!aoMap->loadFromFile(aoPath)) {
        delete aoMap;
        aoMap = nullptr;
        std::cout << "Warning: Could not load AO map for " << baseName << std::endl;
    }
    
    // Try to load height map
    std::string heightPath = "textures/" + baseName + "_height.png";
    heightMap = new Texture();
    if (!heightMap->loadFromFile(heightPath)) {
        delete heightMap;
        heightMap = nullptr;
        std::cout << "Warning: Could not load height map for " << baseName << std::endl;
    }
    
    // For now, we don't load metallic separately - we'll use a default value
    // You could add metallic map loading here if needed
    
    return success;
}

void Material::bindTextures() const {
    if (albedoMap) {
        albedoMap->bind(0);
    }
    if (normalMap) {
        normalMap->bind(1);
    }
    if (roughnessMap) {
        roughnessMap->bind(2);
    }
    if (metallicMap) {
        metallicMap->bind(3);
    }
    if (aoMap) {
        aoMap->bind(4);
    }
    if (heightMap) {
        heightMap->bind(5);
    }
}

void Material::unbindTextures() const {
    for (int i = 0; i < 6; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Material::setUniforms(unsigned int shaderId) const {
    // Use OpenGL uniform functions directly instead of creating a Shader object
    
    // Set material properties
    glUniform1i(glGetUniformLocation(shaderId, "hasMaterial"), true);
    glUniform3fv(glGetUniformLocation(shaderId, "materialBaseColor"), 1, &baseColor[0]);
    glUniform1f(glGetUniformLocation(shaderId, "materialRoughness"), roughness);
    glUniform1f(glGetUniformLocation(shaderId, "materialMetallic"), metallic);
    glUniform1f(glGetUniformLocation(shaderId, "materialAO"), ambientOcclusion);
    
    // Set texture flags and bind textures
    glUniform1i(glGetUniformLocation(shaderId, "hasAlbedoMap"), albedoMap != nullptr);
    glUniform1i(glGetUniformLocation(shaderId, "hasNormalMap"), normalMap != nullptr);
    glUniform1i(glGetUniformLocation(shaderId, "hasRoughnessMap"), roughnessMap != nullptr);
    glUniform1i(glGetUniformLocation(shaderId, "hasMetallicMap"), metallicMap != nullptr);
    glUniform1i(glGetUniformLocation(shaderId, "hasAOMap"), aoMap != nullptr);
    
    // Bind textures and set samplers
    if (albedoMap) {
        glUniform1i(glGetUniformLocation(shaderId, "albedoMap"), 0);
        albedoMap->bind(0);
    }
    if (normalMap) {
        glUniform1i(glGetUniformLocation(shaderId, "normalMap"), 1);
        normalMap->bind(1);
    }
    if (roughnessMap) {
        glUniform1i(glGetUniformLocation(shaderId, "roughnessMap"), 2);
        roughnessMap->bind(2);
    }
    if (metallicMap) {
        glUniform1i(glGetUniformLocation(shaderId, "metallicMap"), 3);
        metallicMap->bind(3);
    }
    if (aoMap) {
        glUniform1i(glGetUniformLocation(shaderId, "aoMap"), 4);
        aoMap->bind(4);
    }
} 