#ifndef OPENGL_DEVICE_H
#define OPENGL_DEVICE_H

#include "GraphicsAPI.h"
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <unordered_map>

// OpenGL Texture Implementation
class OpenGLTexture : public ITexture {
private:
    GLuint textureID;
    int width, height;
    TextureFormat format;
    
public:
    OpenGLTexture(int w, int h, TextureFormat fmt);
    ~OpenGLTexture();
    
    void bind(int unit = 0) override;
    void unbind() override;
    int getWidth() const override { return width; }
    int getHeight() const override { return height; }
    TextureFormat getFormat() const override { return format; }
    unsigned int getHandle() const override { return textureID; }
    
    static GLenum formatToGL(TextureFormat format);
    static GLenum formatToGLInternal(TextureFormat format);
    static GLenum formatToGLType(TextureFormat format);
};

// OpenGL Framebuffer Implementation
class OpenGLFramebuffer : public IFramebuffer {
private:
    GLuint framebufferID;
    std::vector<std::shared_ptr<ITexture>> colorAttachments;
    std::shared_ptr<ITexture> depthAttachment;
    
public:
    OpenGLFramebuffer();
    ~OpenGLFramebuffer();
    
    void bind() override;
    void unbind() override;
    void attachTexture(std::shared_ptr<ITexture> texture, int attachment = 0) override;
    void attachDepthTexture(std::shared_ptr<ITexture> texture) override;
    bool isComplete() const override;
    unsigned int getHandle() const override { return framebufferID; }
};

// OpenGL Shader Implementation
class OpenGLShader : public IShader {
private:
    GLuint programID;
    std::unordered_map<std::string, GLint> uniformLocations;
    
    GLint getUniformLocation(const std::string& name);
    std::string loadShaderSource(const std::string& path);
    GLuint compileShader(GLenum type, const std::string& source);
    
public:
    OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath);
    ~OpenGLShader();
    
    void use() override;
    void setInt(const std::string& name, int value) override;
    void setFloat(const std::string& name, float value) override;
    void setVec2(const std::string& name, const glm::vec2& value) override;
    void setVec3(const std::string& name, const glm::vec3& value) override;
    void setVec4(const std::string& name, const glm::vec4& value) override;
    void setMat3(const std::string& name, const glm::mat3& value) override;
    void setMat4(const std::string& name, const glm::mat4& value) override;
    unsigned int getProgram() const override { return programID; }
};

// OpenGL Buffer Implementation
class OpenGLBuffer : public IBuffer {
private:
    GLuint bufferID;
    GLenum target;
    
public:
    OpenGLBuffer(GLenum bufferTarget = GL_ARRAY_BUFFER);
    ~OpenGLBuffer();
    
    void bind() override;
    void unbind() override;
    void setData(const void* data, size_t size) override;
    void setSubData(const void* data, size_t offset, size_t size) override;
    unsigned int getHandle() const override { return bufferID; }
};

// OpenGL Graphics Device Implementation
class OpenGLDevice : public IGraphicsDevice {
private:
    GLFWwindow* window;
    GLuint quadVAO, quadVBO;
    bool initialized;
    
    void setupFullscreenQuad();
    
public:
    OpenGLDevice(GLFWwindow* glfwWindow);
    ~OpenGLDevice();
    
    // Resource creation
    std::shared_ptr<ITexture> createTexture(int width, int height, TextureFormat format) override;
    std::shared_ptr<IFramebuffer> createFramebuffer() override;
    std::shared_ptr<IShader> createShader(const std::string& vertexPath, const std::string& fragmentPath) override;
    std::shared_ptr<IBuffer> createBuffer() override;
    
    // Rendering commands
    void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) override;
    void clearDepth() override;
    void setViewport(int x, int y, int width, int height) override;
    void enableDepthTest(bool enable) override;
    void setDepthFunction(int func) override;
    
    // State management
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    
    // Query capabilities
    GraphicsAPIType getAPIType() const override { return GraphicsAPIType::OpenGL; }
    std::string getDeviceName() const override;
    bool isSupported() const override;
    
    // For fullscreen quad rendering
    void renderFullscreenQuad() override;
    
    // OpenGL specific methods
    GLFWwindow* getGLFWWindow() const { return window; }
};

#endif // OPENGL_DEVICE_H 