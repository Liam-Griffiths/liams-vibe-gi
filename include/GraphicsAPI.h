#ifndef GRAPHICS_API_H
#define GRAPHICS_API_H

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#define METAL_AVAILABLE 1
#endif
#endif

/**
 * Abstract Graphics API Interface
 * 
 * Provides a unified interface for both OpenGL and Metal rendering backends.
 * This allows the RadianceCascades implementation to work with either API
 * transparently, choosing the best one at runtime based on platform capabilities.
 */

enum class GraphicsAPIType {
    OpenGL,
    Metal
};

enum class TextureFormat {
    RGBA8,
    RGBA16F,
    RGBA32F,
    RG16F,
    R16F,
    Depth24Stencil8
};

enum class TextureFilter {
    Nearest,
    Linear
};

enum class TextureWrap {
    Repeat,
    ClampToEdge
};

// Abstract texture interface
class ITexture {
public:
    virtual ~ITexture() = default;
    virtual void bind(int unit = 0) = 0;
    virtual void unbind() = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
    virtual unsigned int getHandle() const = 0; // For OpenGL compatibility
};

// Abstract framebuffer interface
class IFramebuffer {
public:
    virtual ~IFramebuffer() = default;
    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void attachTexture(std::shared_ptr<ITexture> texture, int attachment = 0) = 0;
    virtual void attachDepthTexture(std::shared_ptr<ITexture> texture) = 0;
    virtual bool isComplete() const = 0;
    virtual unsigned int getHandle() const = 0; // For OpenGL compatibility
};

// Abstract shader interface
class IShader {
public:
    virtual ~IShader() = default;
    virtual void use() = 0;
    virtual void setInt(const std::string& name, int value) = 0;
    virtual void setFloat(const std::string& name, float value) = 0;
    virtual void setVec2(const std::string& name, const glm::vec2& value) = 0;
    virtual void setVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void setVec4(const std::string& name, const glm::vec4& value) = 0;
    virtual void setMat3(const std::string& name, const glm::mat3& value) = 0;
    virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
    virtual unsigned int getProgram() const = 0; // For OpenGL compatibility
};

// Abstract buffer interface
class IBuffer {
public:
    virtual ~IBuffer() = default;
    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual void setData(const void* data, size_t size) = 0;
    virtual void setSubData(const void* data, size_t offset, size_t size) = 0;
    virtual unsigned int getHandle() const = 0; // For OpenGL compatibility
};

// Graphics device context interface
class IGraphicsDevice {
public:
    virtual ~IGraphicsDevice() = default;
    
    // Resource creation
    virtual std::shared_ptr<ITexture> createTexture(int width, int height, TextureFormat format) = 0;
    virtual std::shared_ptr<IFramebuffer> createFramebuffer() = 0;
    virtual std::shared_ptr<IShader> createShader(const std::string& vertexPath, const std::string& fragmentPath) = 0;
    virtual std::shared_ptr<IBuffer> createBuffer() = 0;
    
    // Rendering commands
    virtual void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) = 0;
    virtual void clearDepth() = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void enableDepthTest(bool enable) = 0;
    virtual void setDepthFunction(int func) = 0;
    
    // State management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;
    
    // Query capabilities
    virtual GraphicsAPIType getAPIType() const = 0;
    virtual std::string getDeviceName() const = 0;
    virtual bool isSupported() const = 0;
    
    // For fullscreen quad rendering
    virtual void renderFullscreenQuad() = 0;
};

// Factory for creating graphics devices
class GraphicsDeviceFactory {
public:
    static std::shared_ptr<IGraphicsDevice> createDevice(GraphicsAPIType preferredAPI = GraphicsAPIType::Metal);
    static GraphicsAPIType getBestAvailableAPI();
    static bool isMetalAvailable();
    static bool isOpenGLAvailable();
};

// Utility class for graphics API detection and capabilities
class GraphicsCapabilities {
public:
    struct DeviceInfo {
        GraphicsAPIType api;
        std::string deviceName;
        std::string driverVersion;
        bool supported;
        int maxTextureSize;
        int maxFramebufferAttachments;
        bool supportsCompute;
        bool supports16BitFloat;
        bool supports32BitFloat;
    };
    
    static std::vector<DeviceInfo> getAvailableDevices();
    static DeviceInfo getBestDevice();
    static bool isHighPerformanceGPUAvailable();
};

#endif // GRAPHICS_API_H 