#include "../include/MetalDevice.h"

#ifdef METAL_AVAILABLE

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <iostream>
#include <fstream>
#include <sstream>

// For GLFW Cocoa integration
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

// Helper function to get NSView from GLFW window
NSView* getNSViewFromGLFWWindow(GLFWwindow* window) {
    return [[glfwGetCocoaWindow(window) contentView] retain];
}

// Metal Texture Implementation
MetalTexture::MetalTexture(id<MTLDevice> device, int w, int h, TextureFormat fmt) 
    : width(w), height(h), format(fmt) {
    
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(MTLPixelFormat)formatToMetal(fmt)
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    
    metalTexture = [device newTextureWithDescriptor:descriptor];
    if (!metalTexture) {
        std::cerr << "Failed to create Metal texture" << std::endl;
    }
}

MetalTexture::~MetalTexture() {
    if (metalTexture) {
        [metalTexture release];
        metalTexture = nil;
    }
}

void MetalTexture::bind(int unit) {
    // Metal doesn't have explicit binding like OpenGL
    // The texture will be bound when setting up the render encoder
}

void MetalTexture::unbind() {
    // No-op for Metal
}

unsigned int MetalTexture::getHandle() const {
    return 0; // Metal doesn't use integer handles
}

int MetalTexture::formatToMetal(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:
            return MTLPixelFormatRGBA8Unorm;
        case TextureFormat::RGBA16F:
            return MTLPixelFormatRGBA16Float;
        case TextureFormat::RGBA32F:
            return MTLPixelFormatRGBA32Float;
        case TextureFormat::RG16F:
            return MTLPixelFormatRG16Float;
        case TextureFormat::R16F:
            return MTLPixelFormatR16Float;
        case TextureFormat::Depth24Stencil8:
            return MTLPixelFormatDepth24Unorm_Stencil8;
        default:
            return MTLPixelFormatRGBA8Unorm;
    }
}

// Metal Framebuffer Implementation
MetalFramebuffer::MetalFramebuffer() {
    renderPassDescriptor = (void*)[[MTLRenderPassDescriptor alloc] init];
}

MetalFramebuffer::~MetalFramebuffer() {
    if (renderPassDescriptor) {
        [(MTLRenderPassDescriptor*)renderPassDescriptor release];
        renderPassDescriptor = nullptr;
    }
}

void MetalFramebuffer::bind() {
    // Metal uses render pass descriptors instead of binding framebuffers
    // This will be handled when beginning a render pass
}

void MetalFramebuffer::unbind() {
    // No-op for Metal
}

void MetalFramebuffer::attachTexture(std::shared_ptr<ITexture> texture, int attachment) {
    if (attachment < colorAttachments.size()) {
        colorAttachments[attachment] = texture;
    } else {
        colorAttachments.resize(attachment + 1);
        colorAttachments[attachment] = texture;
    }
    
    auto metalTexture = std::dynamic_pointer_cast<MetalTexture>(texture);
    if (metalTexture && renderPassDescriptor) {
        MTLRenderPassDescriptor* rpd = (MTLRenderPassDescriptor*)renderPassDescriptor;
        rpd.colorAttachments[attachment].texture = (id<MTLTexture>)metalTexture->getMetalTexture();
        rpd.colorAttachments[attachment].loadAction = MTLLoadActionClear;
        rpd.colorAttachments[attachment].storeAction = MTLStoreActionStore;
        rpd.colorAttachments[attachment].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    }
}

void MetalFramebuffer::attachDepthTexture(std::shared_ptr<ITexture> texture) {
    depthAttachment = texture;
    
    auto metalTexture = std::dynamic_pointer_cast<MetalTexture>(texture);
    if (metalTexture && renderPassDescriptor) {
        MTLRenderPassDescriptor* rpd = (MTLRenderPassDescriptor*)renderPassDescriptor;
        rpd.depthAttachment.texture = (id<MTLTexture>)metalTexture->getMetalTexture();
        rpd.depthAttachment.loadAction = MTLLoadActionClear;
        rpd.depthAttachment.storeAction = MTLStoreActionStore;
        rpd.depthAttachment.clearDepth = 1.0;
    }
}

bool MetalFramebuffer::isComplete() const {
    // Metal render pass descriptors are always "complete" if they have valid textures
    return renderPassDescriptor != nil;
}

unsigned int MetalFramebuffer::getHandle() const {
    return 0; // Metal doesn't use integer handles
}

// Metal Shader Implementation
MetalShader::MetalShader(id<MTLDevice> metalDevice, const std::string& vertexPath, const std::string& fragmentPath)
    : device(metalDevice) {
    
    // Load and compile shaders
    std::string vertexSource = loadShaderSource(vertexPath);
    std::string fragmentSource = loadShaderSource(fragmentPath);
    
    // For this implementation, we'll assume the shaders are already in MSL format
    // In a real implementation, you'd want to convert GLSL to MSL or load .metal files
    
    NSError* error = nil;
    NSString* shaderSource = [NSString stringWithFormat:@"%s\n%s", 
                             vertexSource.c_str(), fragmentSource.c_str()];
    
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
    if (error) {
        std::cerr << "Metal shader compilation error: " << [[error localizedDescription] UTF8String] << std::endl;
        return;
    }
    
    vertexFunction = [library newFunctionWithName:@"vertex_main"];
    fragmentFunction = [library newFunctionWithName:@"fragment_main"];
    
    if (!vertexFunction || !fragmentFunction) {
        std::cerr << "Failed to find Metal shader functions" << std::endl;
        return;
    }
    
    // Create render pipeline state
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    [pipelineDescriptor release];
    
    if (error) {
        std::cerr << "Metal pipeline state creation error: " << [[error localizedDescription] UTF8String] << std::endl;
    }
    
    // Create uniform buffer
    uniformBuffer.size = 4096; // Initial size
    uniformBuffer.buffer = [device newBufferWithLength:uniformBuffer.size options:MTLResourceStorageModeShared];
    uniformBuffer.data = [uniformBuffer.buffer contents];
    
    [library release];
}

MetalShader::~MetalShader() {
    if (pipelineState) {
        [pipelineState release];
        pipelineState = nil;
    }
    if (vertexFunction) {
        [vertexFunction release];
        vertexFunction = nil;
    }
    if (fragmentFunction) {
        [fragmentFunction release];
        fragmentFunction = nil;
    }
    if (uniformBuffer.buffer) {
        [uniformBuffer.buffer release];
        uniformBuffer.buffer = nil;
    }
}

void MetalShader::use() {
    // Metal shaders are used when setting the render pipeline state on the encoder
    // This is handled in the MetalDevice's render methods
}

std::string MetalShader::loadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

void* MetalShader::compileShader(const std::string& source, const std::string& functionName) {
    NSError* error = nil;
    NSString* shaderSource = [NSString stringWithUTF8String:source.c_str()];
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
    
    if (error) {
        std::cerr << "Metal shader compilation error: " << [[error localizedDescription] UTF8String] << std::endl;
        return nullptr;
    }
    
    NSString* funcName = [NSString stringWithUTF8String:functionName.c_str()];
    id<MTLFunction> function = [library newFunctionWithName:funcName];
    [library release];
    
    return (void*)function;
}

void MetalShader::setInt(const std::string& name, int value) {
    // For simplicity, we'll store uniforms in the buffer and update them
    // A real implementation would use proper uniform management
    memcpy(uniformBuffer.data, &value, sizeof(int));
}

void MetalShader::setFloat(const std::string& name, float value) {
    memcpy(uniformBuffer.data, &value, sizeof(float));
}

void MetalShader::setVec2(const std::string& name, const glm::vec2& value) {
    memcpy(uniformBuffer.data, &value, sizeof(glm::vec2));
}

void MetalShader::setVec3(const std::string& name, const glm::vec3& value) {
    memcpy(uniformBuffer.data, &value, sizeof(glm::vec3));
}

void MetalShader::setVec4(const std::string& name, const glm::vec4& value) {
    memcpy(uniformBuffer.data, &value, sizeof(glm::vec4));
}

void MetalShader::setMat3(const std::string& name, const glm::mat3& value) {
    memcpy(uniformBuffer.data, &value, sizeof(glm::mat3));
}

void MetalShader::setMat4(const std::string& name, const glm::mat4& value) {
    memcpy(uniformBuffer.data, &value, sizeof(glm::mat4));
}

unsigned int MetalShader::getProgram() const {
    return 0; // Metal doesn't use integer program handles
}

// Metal Buffer Implementation
MetalBuffer::MetalBuffer(id<MTLDevice> metalDevice, size_t size) 
    : device(metalDevice), bufferSize(size) {
    
    if (size > 0) {
        metalBuffer = [device newBufferWithLength:size options:MTLResourceStorageModeShared];
    }
}

MetalBuffer::~MetalBuffer() {
    if (metalBuffer) {
        [metalBuffer release];
        metalBuffer = nil;
    }
}

void MetalBuffer::bind() {
    // Metal buffers are bound when setting up the render encoder
}

void MetalBuffer::unbind() {
    // No-op for Metal
}

void MetalBuffer::setData(const void* data, size_t size) {
    if (size > bufferSize || !metalBuffer) {
        // Reallocate buffer if needed
        if (metalBuffer) {
            [metalBuffer release];
        }
        metalBuffer = [device newBufferWithLength:size options:MTLResourceStorageModeShared];
        bufferSize = size;
    }
    
    if (metalBuffer && data) {
        memcpy([metalBuffer contents], data, size);
    }
}

void MetalBuffer::setSubData(const void* data, size_t offset, size_t size) {
    if (metalBuffer && data && (offset + size) <= bufferSize) {
        void* bufferContents = [metalBuffer contents];
        memcpy((char*)bufferContents + offset, data, size);
    }
}

unsigned int MetalBuffer::getHandle() const {
    return 0; // Metal doesn't use integer handles
}

// Metal Graphics Device Implementation
MetalDevice::MetalDevice(GLFWwindow* glfwWindow) 
    : window(glfwWindow), device(nil), commandQueue(nil), metalLayer(nil), metalView(nil),
      quadVertexBuffer(nil), quadPipelineState(nil), currentCommandBuffer(nil), 
      currentRenderEncoder(nil), inRenderPass(false) {
    
    if (initializeMetal()) {
        setupMetalLayer();
        setupFullscreenQuad();
    }
}

MetalDevice::~MetalDevice() {
    if (currentRenderEncoder) {
        [currentRenderEncoder release];
        currentRenderEncoder = nil;
    }
    if (currentCommandBuffer) {
        [currentCommandBuffer release];
        currentCommandBuffer = nil;
    }
    if (quadVertexBuffer) {
        [quadVertexBuffer release];
        quadVertexBuffer = nil;
    }
    if (quadPipelineState) {
        [quadPipelineState release];
        quadPipelineState = nil;
    }
    if (commandQueue) {
        [commandQueue release];
        commandQueue = nil;
    }
    if (device) {
        [device release];
        device = nil;
    }
    if (metalLayer) {
        [metalLayer release];
        metalLayer = nil;
    }
}

bool MetalDevice::initializeMetal() {
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "Metal is not supported on this device" << std::endl;
        return false;
    }
    
    commandQueue = [device newCommandQueue];
    if (!commandQueue) {
        std::cerr << "Failed to create Metal command queue" << std::endl;
        return false;
    }
    
    return true;
}

void MetalDevice::setupMetalLayer() {
    NSView* contentView = getNSViewFromGLFWWindow(window);
    if (!contentView) {
        std::cerr << "Failed to get NSView from GLFW window" << std::endl;
        return;
    }
    
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = device;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
    
    // Set layer frame to match window size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    metalLayer.drawableSize = CGSizeMake(width, height);
    
    contentView.layer = metalLayer;
    contentView.wantsLayer = YES;
}

void MetalDevice::setupFullscreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    quadVertexBuffer = [device newBufferWithBytes:quadVertices 
                                           length:sizeof(quadVertices) 
                                          options:MTLResourceStorageModeShared];
}

std::shared_ptr<ITexture> MetalDevice::createTexture(int width, int height, TextureFormat format) {
    return std::make_shared<MetalTexture>(device, width, height, format);
}

std::shared_ptr<IFramebuffer> MetalDevice::createFramebuffer() {
    return std::make_shared<MetalFramebuffer>();
}

std::shared_ptr<IShader> MetalDevice::createShader(const std::string& vertexPath, const std::string& fragmentPath) {
    return std::make_shared<MetalShader>(device, vertexPath, fragmentPath);
}

std::shared_ptr<IBuffer> MetalDevice::createBuffer() {
    return std::make_shared<MetalBuffer>(device);
}

void MetalDevice::clear(float r, float g, float b, float a) {
    // Metal clears are handled by the render pass descriptor
    // This would be set when beginning a render pass
}

void MetalDevice::clearDepth() {
    // Metal depth clears are handled by the render pass descriptor
}

void MetalDevice::setViewport(int x, int y, int width, int height) {
    // Metal viewports are set on the render command encoder
    if (currentRenderEncoder) {
        MTLViewport viewport = {
            .originX = (double)x,
            .originY = (double)y,
            .width = (double)width,
            .height = (double)height,
            .znear = 0.0,
            .zfar = 1.0
        };
        [currentRenderEncoder setViewport:viewport];
    }
}

void MetalDevice::enableDepthTest(bool enable) {
    // Metal depth testing is configured in the render pipeline state
    // This would need to be handled when creating the pipeline
}

void MetalDevice::setDepthFunction(int func) {
    // Metal depth function is configured in the render pipeline state
}

void MetalDevice::beginFrame() {
    if (!commandQueue) return;
    
    currentCommandBuffer = [commandQueue commandBuffer];
    if (!currentCommandBuffer) {
        std::cerr << "Failed to create Metal command buffer" << std::endl;
    }
}

void MetalDevice::endFrame() {
    if (currentRenderEncoder) {
        [currentRenderEncoder endEncoding];
        [currentRenderEncoder release];
        currentRenderEncoder = nil;
        inRenderPass = false;
    }
    
    if (currentCommandBuffer) {
        [currentCommandBuffer commit];
        [currentCommandBuffer release];
        currentCommandBuffer = nil;
    }
}

void MetalDevice::present() {
    if (currentCommandBuffer && metalLayer) {
        id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
        if (drawable) {
            [currentCommandBuffer presentDrawable:drawable];
        }
    }
}

std::string MetalDevice::getDeviceName() const {
    if (device) {
        return std::string([device.name UTF8String]);
    }
    return "Unknown Metal Device";
}

bool MetalDevice::isSupported() const {
    return device != nil && commandQueue != nil;
}

void MetalDevice::renderFullscreenQuad() {
    if (currentRenderEncoder && quadVertexBuffer) {
        [currentRenderEncoder setVertexBuffer:quadVertexBuffer offset:0 atIndex:0];
        [currentRenderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }
}

void MetalDevice::beginRenderPass(MTLRenderPassDescriptor* renderPassDescriptor) {
    if (currentCommandBuffer && !inRenderPass) {
        currentRenderEncoder = [currentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        inRenderPass = true;
    }
}

void MetalDevice::endRenderPass() {
    if (currentRenderEncoder && inRenderPass) {
        [currentRenderEncoder endEncoding];
        [currentRenderEncoder release];
        currentRenderEncoder = nil;
        inRenderPass = false;
    }
}

#endif // METAL_AVAILABLE 