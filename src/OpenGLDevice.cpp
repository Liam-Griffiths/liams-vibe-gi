#include "../include/OpenGLDevice.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// OpenGL Texture Implementation
OpenGLTexture::OpenGLTexture(int w, int h, TextureFormat fmt) : width(w), height(h), format(fmt) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    GLenum internalFormat = formatToGLInternal(fmt);
    GLenum dataFormat = formatToGL(fmt);
    GLenum dataType = formatToGLType(fmt);
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

OpenGLTexture::~OpenGLTexture() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }
}

void OpenGLTexture::bind(int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void OpenGLTexture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLenum OpenGLTexture::formatToGL(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:
            return GL_RGBA;
        case TextureFormat::RG16F:
            return GL_RG;
        case TextureFormat::R16F:
            return GL_RED;
        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH_STENCIL;
        default:
            return GL_RGBA;
    }
}

GLenum OpenGLTexture::formatToGLInternal(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:
            return GL_RGBA8;
        case TextureFormat::RGBA16F:
            return GL_RGBA16F;
        case TextureFormat::RGBA32F:
            return GL_RGBA32F;
        case TextureFormat::RG16F:
            return GL_RG16F;
        case TextureFormat::R16F:
            return GL_R16F;
        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH24_STENCIL8;
        default:
            return GL_RGBA8;
    }
}

GLenum OpenGLTexture::formatToGLType(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::RGBA16F:
        case TextureFormat::RG16F:
        case TextureFormat::R16F:
            return GL_HALF_FLOAT;
        case TextureFormat::RGBA32F:
            return GL_FLOAT;
        case TextureFormat::Depth24Stencil8:
            return GL_UNSIGNED_INT_24_8;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

// OpenGL Framebuffer Implementation
OpenGLFramebuffer::OpenGLFramebuffer() {
    glGenFramebuffers(1, &framebufferID);
}

OpenGLFramebuffer::~OpenGLFramebuffer() {
    if (framebufferID) {
        glDeleteFramebuffers(1, &framebufferID);
    }
}

void OpenGLFramebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
}

void OpenGLFramebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::attachTexture(std::shared_ptr<ITexture> texture, int attachment) {
    bind();
    
    if (attachment < colorAttachments.size()) {
        colorAttachments[attachment] = texture;
    } else {
        colorAttachments.resize(attachment + 1);
        colorAttachments[attachment] = texture;
    }
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, 
                          GL_TEXTURE_2D, texture->getHandle(), 0);
}

void OpenGLFramebuffer::attachDepthTexture(std::shared_ptr<ITexture> texture) {
    bind();
    depthAttachment = texture;
    
    if (texture->getFormat() == TextureFormat::Depth24Stencil8) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 
                              GL_TEXTURE_2D, texture->getHandle(), 0);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                              GL_TEXTURE_2D, texture->getHandle(), 0);
    }
}

bool OpenGLFramebuffer::isComplete() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

// OpenGL Shader Implementation
OpenGLShader::OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = loadShaderSource(vertexPath);
    std::string fragmentSource = loadShaderSource(fragmentPath);
    
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);
    
    // Check for linking errors
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programID, 1024, nullptr, infoLog);
        std::cerr << "Shader linking error: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

OpenGLShader::~OpenGLShader() {
    if (programID) {
        glDeleteProgram(programID);
    }
}

void OpenGLShader::use() {
    glUseProgram(programID);
}

std::string OpenGLShader::loadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint OpenGLShader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    // Check for compilation errors
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << typeStr << " shader compilation error: " << infoLog << std::endl;
    }
    
    return shader;
}

GLint OpenGLShader::getUniformLocation(const std::string& name) {
    if (uniformLocations.find(name) != uniformLocations.end()) {
        return uniformLocations[name];
    }
    
    GLint location = glGetUniformLocation(programID, name.c_str());
    uniformLocations[name] = location;
    return location;
}

void OpenGLShader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void OpenGLShader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void OpenGLShader::setVec2(const std::string& name, const glm::vec2& value) {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void OpenGLShader::setVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void OpenGLShader::setVec4(const std::string& name, const glm::vec4& value) {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void OpenGLShader::setMat3(const std::string& name, const glm::mat3& value) {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::setMat4(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// OpenGL Buffer Implementation
OpenGLBuffer::OpenGLBuffer(GLenum bufferTarget) : target(bufferTarget) {
    glGenBuffers(1, &bufferID);
}

OpenGLBuffer::~OpenGLBuffer() {
    if (bufferID) {
        glDeleteBuffers(1, &bufferID);
    }
}

void OpenGLBuffer::bind() {
    glBindBuffer(target, bufferID);
}

void OpenGLBuffer::unbind() {
    glBindBuffer(target, 0);
}

void OpenGLBuffer::setData(const void* data, size_t size) {
    bind();
    glBufferData(target, size, data, GL_STATIC_DRAW);
}

void OpenGLBuffer::setSubData(const void* data, size_t offset, size_t size) {
    bind();
    glBufferSubData(target, offset, size, data);
}

// OpenGL Graphics Device Implementation
OpenGLDevice::OpenGLDevice(GLFWwindow* glfwWindow) 
    : window(glfwWindow), quadVAO(0), quadVBO(0), initialized(false) {
    if (window && glfwGetCurrentContext() == window) {
        setupFullscreenQuad();
        initialized = true;
    }
}

OpenGLDevice::~OpenGLDevice() {
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
}

void OpenGLDevice::setupFullscreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

std::shared_ptr<ITexture> OpenGLDevice::createTexture(int width, int height, TextureFormat format) {
    return std::make_shared<OpenGLTexture>(width, height, format);
}

std::shared_ptr<IFramebuffer> OpenGLDevice::createFramebuffer() {
    return std::make_shared<OpenGLFramebuffer>();
}

std::shared_ptr<IShader> OpenGLDevice::createShader(const std::string& vertexPath, const std::string& fragmentPath) {
    return std::make_shared<OpenGLShader>(vertexPath, fragmentPath);
}

std::shared_ptr<IBuffer> OpenGLDevice::createBuffer() {
    return std::make_shared<OpenGLBuffer>();
}

void OpenGLDevice::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLDevice::clearDepth() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLDevice::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void OpenGLDevice::enableDepthTest(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void OpenGLDevice::setDepthFunction(int func) {
    glDepthFunc(func);
}

void OpenGLDevice::beginFrame() {
    // OpenGL doesn't need explicit frame begin
}

void OpenGLDevice::endFrame() {
    // OpenGL doesn't need explicit frame end
}

void OpenGLDevice::present() {
    if (window) {
        glfwSwapBuffers(window);
    }
}

std::string OpenGLDevice::getDeviceName() const {
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    return renderer ? std::string(renderer) : "Unknown OpenGL Device";
}

bool OpenGLDevice::isSupported() const {
    return initialized && window && glfwGetCurrentContext() == window;
}

void OpenGLDevice::renderFullscreenQuad() {
    if (quadVAO) {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
} 