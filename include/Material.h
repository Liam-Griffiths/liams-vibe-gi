/**
 * Material.h - Physically Based Rendering (PBR) Material System
 * 
 * Implements a complete PBR material system with texture support for realistic
 * material representation. The system supports the metallic-roughness workflow
 * which is standard in modern real-time rendering.
 * 
 * PBR Material Model:
 * - Base Color (Albedo): The primary color/texture of the material
 * - Roughness: Surface microsurface roughness (0 = mirror, 1 = completely rough)
 * - Metallic: Whether surface is metallic (0 = dielectric, 1 = metal)
 * - Ambient Occlusion: Contact shadows for enhanced depth perception
 * - Normal Map: Surface detail without additional geometry
 * - Height Map: Displacement information for parallax effects
 * 
 * Features:
 * - Automatic PBR texture loading from standard naming conventions
 * - OpenGL texture management with proper binding/unbinding
 * - Shader uniform integration for rendering
 * - Memory management with proper cleanup
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <glm/glm.hpp>

/**
 * Texture class - OpenGL texture wrapper for image data
 * 
 * Manages loading, binding, and cleanup of OpenGL textures from image files.
 * Supports common image formats (PNG, JPG, etc.) and provides proper
 * OpenGL state management.
 */
class Texture {
public:
    unsigned int id;        ///< OpenGL texture ID
    std::string type;       ///< Texture type (albedo, normal, roughness, etc.)
    std::string path;       ///< File path for loading
    
    /**
     * Default constructor - Creates uninitialized texture
     */
    Texture();
    
    /**
     * Constructor with immediate loading
     * 
     * @param path Filesystem path to image file
     * @param type Texture type identifier (for shader binding)
     */
    Texture(const std::string& path, const std::string& type);
    
    /**
     * Destructor - Automatically cleans up OpenGL texture
     */
    ~Texture();
    
    /**
     * Load texture data from image file
     * Supports PNG, JPG, and other common formats via stb_image
     * 
     * @param path Path to image file
     * @return true if loading succeeded, false otherwise
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * Bind texture to specified texture unit for rendering
     * 
     * @param unit OpenGL texture unit (0-31, default: 0)
     */
    void bind(unsigned int unit = 0) const;
    
    /**
     * Unbind texture from current texture unit
     */
    void unbind() const;
    
private:
    int width, height, nrChannels;  ///< Image dimensions and channel count
};

/**
 * Material class - Complete PBR material with textures and properties
 * 
 * Implements the metallic-roughness PBR workflow with full texture support.
 * Materials can be created procedurally or loaded from texture files following
 * standard naming conventions (basecolor, normal, roughness, etc.).
 */
class Material {
public:
    // PBR Material Properties (base values, can be overridden by textures)
    glm::vec3 baseColor;        ///< Base albedo color (RGB, 0-1 range)
    float roughness;            ///< Surface roughness (0 = mirror, 1 = rough)
    float metallic;             ///< Metallic property (0 = dielectric, 1 = metal)
    float ambientOcclusion;     ///< Ambient occlusion factor (0 = full shadow, 1 = no shadow)
    
    // Texture mapping properties
    glm::vec2 tiling;           ///< Texture coordinate scaling for tiling (default: 1,1)
    float heightScale;          ///< Height map displacement scale (default: 0.02)
    
    // PBR Texture Maps (nullptr if not used)
    Texture* albedoMap;         ///< Base color texture (RGB)
    Texture* normalMap;         ///< Normal map for surface detail (RGB, tangent space)
    Texture* roughnessMap;      ///< Roughness values (R channel typically used)
    Texture* metallicMap;       ///< Metallic values (R channel typically used)
    Texture* aoMap;             ///< Ambient occlusion texture (R channel)
    Texture* heightMap;         ///< Height/displacement map (R channel)
    
    /**
     * Default constructor - Creates basic material with default properties
     */
    Material();
    
    /**
     * Constructor with basic material properties
     * 
     * @param color     Base color (default: white)
     * @param roughness Surface roughness (default: 0.5)
     * @param metallic  Metallic property (default: 0.0 - non-metal)
     */
    Material(const glm::vec3& color, float roughness = 0.5f, float metallic = 0.0f);
    
    /**
     * Destructor - Cleans up all allocated textures
     */
    ~Material();
    
    /**
     * Load complete PBR material from texture files
     * 
     * Automatically loads textures based on standard naming convention:
     * - baseName_basecolor.* (or baseName_albedo.*)
     * - baseName_normal.*
     * - baseName_roughness.*
     * - baseName_metallic.*
     * - baseName_ambientOcclusion.* (or baseName_ao.*)
     * - baseName_height.*
     * 
     * @param baseName Base filename without extension or suffix
     * @return true if at least one texture was loaded successfully
     */
    bool loadPBRMaterial(const std::string& baseName);
    
    /**
     * Bind all available textures to appropriate texture units
     * Sets up texture units for shader access during rendering
     */
    void bindTextures() const;
    
    /**
     * Unbind all textures from their texture units
     * Cleans up texture state after rendering
     */
    void unbindTextures() const;
    
    /**
     * Set material properties as shader uniforms
     * 
     * Uploads material properties and texture availability flags to shaders.
     * Assumes standard PBR uniform names in the shader program.
     * 
     * @param shaderId OpenGL shader program ID
     */
    void setUniforms(unsigned int shaderId) const;
    
private:
    /**
     * Initialize material with default PBR values
     */
    void initializeDefaults();
};

#endif // MATERIAL_H 