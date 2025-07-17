/**
 * Shader.h - OpenGL Shader Program Management
 * 
 * Provides a convenient wrapper for OpenGL shader programs, handling compilation,
 * linking, and uniform setting operations. Simplifies shader usage throughout
 * the rendering pipeline.
 * 
 * Features:
 * - Automatic vertex and fragment shader compilation
 * - Shader program linking with error checking
 * - Type-safe uniform setting methods
 * - Convenient uniform access by name
 * - Proper error handling and reporting
 * 
 * Usage:
 * - Create shader with vertex and fragment shader files
 * - Use shader.use() to bind for rendering
 * - Set uniforms with type-specific methods
 * - Shader automatically manages OpenGL state
 * 
 * Supported uniform types:
 * - bool, int, float (scalars)
 * - vec2, vec3 (vectors)
 * - mat4 (4x4 matrices)
 */

#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glm/glm.hpp>

/**
 * Shader class - OpenGL shader program wrapper
 * 
 * Manages the lifecycle of OpenGL shader programs from source files.
 * Provides convenient methods for setting uniforms and using shaders
 * in the rendering pipeline.
 */
class Shader {
public:
    unsigned int ID;    ///< OpenGL shader program ID

    /**
     * Constructor - Compile and link shader program from source files
     * 
     * Loads vertex and fragment shaders from files, compiles them,
     * and links into a complete shader program. Reports any compilation
     * or linking errors to the console.
     * 
     * @param vertexPath   Path to vertex shader source file (.vert)
     * @param fragmentPath Path to fragment shader source file (.frag)
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * Activate this shader program for rendering
     * Binds the shader program to the OpenGL context
     */
    void use();
    
    // Uniform Setting Methods
    // These methods provide type-safe uniform setting with automatic
    // uniform location lookup by name
    
    /**
     * Set boolean uniform variable
     * 
     * @param name  Uniform variable name in shader
     * @param value Boolean value to set
     */
    void setBool(const std::string &name, bool value) const;
    
    /**
     * Set integer uniform variable
     * 
     * @param name  Uniform variable name in shader
     * @param value Integer value to set
     */
    void setInt(const std::string &name, int value) const;
    
    /**
     * Set float uniform variable
     * 
     * @param name  Uniform variable name in shader
     * @param value Float value to set
     */
    void setFloat(const std::string &name, float value) const;
    
    /**
     * Set 2D vector uniform variable
     * 
     * @param name  Uniform variable name in shader
     * @param value 2D vector value (vec2 in GLSL)
     */
    void setVec2(const std::string &name, const glm::vec2 &value) const;
    
    /**
     * Set 3D vector uniform variable
     * 
     * @param name  Uniform variable name in shader
     * @param value 3D vector value (vec3 in GLSL)
     */
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    
    /**
     * Set 4x4 matrix uniform variable
     * 
     * @param name Uniform variable name in shader
     * @param mat  4x4 matrix value (mat4 in GLSL)
     */
    void setMat4(const std::string &name, const glm::mat4 &mat) const;

private:
    /**
     * Check shader compilation and program linking errors
     * 
     * Checks for compilation/linking errors and prints detailed
     * error information to help debug shader issues.
     * 
     * @param shader Shader object ID to check
     * @param type   Type of check ("PROGRAM", "VERTEX", "FRAGMENT")
     */
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif // SHADER_H 