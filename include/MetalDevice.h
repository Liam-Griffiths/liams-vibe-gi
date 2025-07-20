#ifndef METAL_DEVICE_H
#define METAL_DEVICE_H

#include "GraphicsAPI.h"

#include <GLFW/glfw3.h>

#ifdef METAL_AVAILABLE
// Forward declarations for Metal types to avoid importing Metal headers in C++
#ifdef __OBJC__
    @class CAMetalLayer;
    @protocol MTLDevice;
    @protocol MTLCommandQueue;
    @protocol MTLTexture;
    @protocol MTLBuffer;
    @protocol MTLRenderPipelineState;
    @protocol MTLCommandBuffer;
    @protocol MTLRenderCommandEncoder;
    @protocol MTLFunction;
    @class MTLRenderPassDescriptor;
#else
    typedef void* id;
    typedef void* CAMetalLayer;
    typedef void* MTLDevice_id;
    typedef void* MTLCommandQueue_id;
    typedef void* MTLTexture_id;
    typedef void* MTLBuffer_id;
    typedef void* MTLRenderPipelineState_id;
    typedef void* MTLCommandBuffer_id;
    typedef void* MTLRenderCommandEncoder_id;
    typedef void* MTLFunction_id;
    typedef void* MTLRenderPassDescriptor_id;
#endif

// Metal Texture Implementation
class MetalTexture : public ITexture {
private:
#ifdef __OBJC__
    id<MTLTexture> metalTexture;
#else
    MTLTexture_id metalTexture;
#endif
    int width, height;
    TextureFormat format;
    
public:
#ifdef __OBJC__
    MetalTexture(id<MTLDevice> device, int w, int h, TextureFormat fmt);
    id<MTLTexture> getMetalTexture() const { return metalTexture; }
#else
    MetalTexture(MTLDevice_id device, int w, int h, TextureFormat fmt);
    MTLTexture_id getMetalTexture() const { return metalTexture; }
#endif
    ~MetalTexture();
    
    void bind(int unit = 0) override;
    void unbind() override;
    int getWidth() const override { return width; }
    int getHeight() const override { return height; }
    TextureFormat getFormat() const override { return format; }
    unsigned int getHandle() const override; // Returns 0 for Metal (not applicable)
    
    static int formatToMetal(TextureFormat format); // Returns MTLPixelFormat as int
};

// Metal Framebuffer Implementation (Render Pass Descriptor wrapper)
class MetalFramebuffer : public IFramebuffer {
private:
    void* renderPassDescriptor; // MTLRenderPassDescriptor*
    std::vector<std::shared_ptr<ITexture>> colorAttachments;
    std::shared_ptr<ITexture> depthAttachment;
    
public:
    MetalFramebuffer();
    ~MetalFramebuffer();
    
    void bind() override;
    void unbind() override;
    void attachTexture(std::shared_ptr<ITexture> texture, int attachment = 0) override;
    void attachDepthTexture(std::shared_ptr<ITexture> texture) override;
    bool isComplete() const override;
    unsigned int getHandle() const override; // Returns 0 for Metal (not applicable)
    
    // Metal specific methods
    void* getRenderPassDescriptor() const { return renderPassDescriptor; }
};

// Metal Shader Implementation (Render Pipeline State wrapper)
class MetalShader : public IShader {
private:
#ifdef __OBJC__
    id<MTLDevice> device;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLFunction> vertexFunction;
    id<MTLFunction> fragmentFunction;
#else
    MTLDevice_id device;
    MTLRenderPipelineState_id pipelineState;
    MTLFunction_id vertexFunction;
    MTLFunction_id fragmentFunction;
#endif
    
    // Uniform buffer management
    struct UniformBuffer {
#ifdef __OBJC__
        id<MTLBuffer> buffer;
#else
        MTLBuffer_id buffer;
#endif
        size_t size;
        void* data;
    };
    UniformBuffer uniformBuffer;
    
    std::string loadShaderSource(const std::string& path);
    void* compileShader(const std::string& source, const std::string& functionName);
    
public:
#ifdef __OBJC__
    MetalShader(id<MTLDevice> metalDevice, const std::string& vertexPath, const std::string& fragmentPath);
    id<MTLRenderPipelineState> getPipelineState() const { return pipelineState; }
    id<MTLBuffer> getUniformBuffer() const { return uniformBuffer.buffer; }
#else
    MetalShader(MTLDevice_id metalDevice, const std::string& vertexPath, const std::string& fragmentPath);
    MTLRenderPipelineState_id getPipelineState() const { return pipelineState; }
    MTLBuffer_id getUniformBuffer() const { return uniformBuffer.buffer; }
#endif
    ~MetalShader();
    
    void use() override;
    void setInt(const std::string& name, int value) override;
    void setFloat(const std::string& name, float value) override;
    void setVec2(const std::string& name, const glm::vec2& value) override;
    void setVec3(const std::string& name, const glm::vec3& value) override;
    void setVec4(const std::string& name, const glm::vec4& value) override;
    void setMat3(const std::string& name, const glm::mat3& value) override;
    void setMat4(const std::string& name, const glm::mat4& value) override;
    unsigned int getProgram() const override; // Returns 0 for Metal (not applicable)
};

// Metal Buffer Implementation
class MetalBuffer : public IBuffer {
private:
#ifdef __OBJC__
    id<MTLBuffer> metalBuffer;
    id<MTLDevice> device;
#else
    MTLBuffer_id metalBuffer;
    MTLDevice_id device;
#endif
    size_t bufferSize;
    
public:
#ifdef __OBJC__
    MetalBuffer(id<MTLDevice> metalDevice, size_t size = 0);
    id<MTLBuffer> getMetalBuffer() const { return metalBuffer; }
#else
    MetalBuffer(MTLDevice_id metalDevice, size_t size = 0);
    MTLBuffer_id getMetalBuffer() const { return metalBuffer; }
#endif
    ~MetalBuffer();
    
    void bind() override;
    void unbind() override;
    void setData(const void* data, size_t size) override;
    void setSubData(const void* data, size_t offset, size_t size) override;
    unsigned int getHandle() const override; // Returns 0 for Metal (not applicable)
};

// Metal Graphics Device Implementation
class MetalDevice : public IGraphicsDevice {
private:
    GLFWwindow* window;
#ifdef __OBJC__
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    CAMetalLayer* metalLayer;
    void* metalView; // MTKView*
    
    // Fullscreen quad resources
    id<MTLBuffer> quadVertexBuffer;
    id<MTLRenderPipelineState> quadPipelineState;
    
    // Current rendering state
    id<MTLCommandBuffer> currentCommandBuffer;
    id<MTLRenderCommandEncoder> currentRenderEncoder;
#else
    MTLDevice_id device;
    MTLCommandQueue_id commandQueue;
    CAMetalLayer metalLayer;
    void* metalView; // MTKView*
    
    // Fullscreen quad resources
    MTLBuffer_id quadVertexBuffer;
    MTLRenderPipelineState_id quadPipelineState;
    
    // Current rendering state
    MTLCommandBuffer_id currentCommandBuffer;
    MTLRenderCommandEncoder_id currentRenderEncoder;
#endif
    bool inRenderPass;
    
    void setupMetalLayer();
    void setupFullscreenQuad();
    bool initializeMetal();
    
public:
    MetalDevice(GLFWwindow* glfwWindow);
    ~MetalDevice();
    
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
    GraphicsAPIType getAPIType() const override { return GraphicsAPIType::Metal; }
    std::string getDeviceName() const override;
    bool isSupported() const override;
    
    // For fullscreen quad rendering
    void renderFullscreenQuad() override;
    
    // Metal specific methods
#ifdef __OBJC__
    id<MTLDevice> getMetalDevice() const { return device; }
    id<MTLCommandQueue> getCommandQueue() const { return commandQueue; }
    id<MTLCommandBuffer> getCurrentCommandBuffer() const { return currentCommandBuffer; }
    id<MTLRenderCommandEncoder> getCurrentRenderEncoder() const { return currentRenderEncoder; }
    void beginRenderPass(MTLRenderPassDescriptor* renderPassDescriptor);
#else
    MTLDevice_id getMetalDevice() const { return device; }
    MTLCommandQueue_id getCommandQueue() const { return commandQueue; }
    MTLCommandBuffer_id getCurrentCommandBuffer() const { return currentCommandBuffer; }
    MTLRenderCommandEncoder_id getCurrentRenderEncoder() const { return currentRenderEncoder; }
    void beginRenderPass(MTLRenderPassDescriptor_id renderPassDescriptor);
#endif
    void endRenderPass();
};

#endif // METAL_AVAILABLE

#endif // METAL_DEVICE_H 