#include "../include/GraphicsAPI.h"
#include "../include/OpenGLDevice.h"

#ifdef METAL_AVAILABLE
#include "../include/MetalDevice.h"
#endif

#include <iostream>
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

// GraphicsDeviceFactory Implementation
std::shared_ptr<IGraphicsDevice> GraphicsDeviceFactory::createDevice(GraphicsAPIType preferredAPI) {
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    
    if (!currentWindow) {
        std::cerr << "No current OpenGL context found. Cannot create graphics device." << std::endl;
        return nullptr;
    }
    
    // Try preferred API first
    if (preferredAPI == GraphicsAPIType::Metal && isMetalAvailable()) {
#ifdef METAL_AVAILABLE
        auto metalDevice = std::make_shared<MetalDevice>(currentWindow);
        if (metalDevice->isSupported()) {
            std::cout << "Created Metal graphics device: " << metalDevice->getDeviceName() << std::endl;
            return metalDevice;
        }
#endif
    }
    
    // Fallback to OpenGL
    if (isOpenGLAvailable()) {
        auto openglDevice = std::make_shared<OpenGLDevice>(currentWindow);
        if (openglDevice->isSupported()) {
            std::cout << "Created OpenGL graphics device: " << openglDevice->getDeviceName() << std::endl;
            return openglDevice;
        }
    }
    
    std::cerr << "Failed to create any graphics device" << std::endl;
    return nullptr;
}

GraphicsAPIType GraphicsDeviceFactory::getBestAvailableAPI() {
#ifdef METAL_AVAILABLE
    if (isMetalAvailable()) {
        return GraphicsAPIType::Metal;
    }
#endif
    
    if (isOpenGLAvailable()) {
        return GraphicsAPIType::OpenGL;
    }
    
    return GraphicsAPIType::OpenGL; // Default fallback
}

bool GraphicsDeviceFactory::isMetalAvailable() {
#ifdef METAL_AVAILABLE
    // Forward declare Metal availability check function (implemented in .mm file)
    extern bool checkMetalAvailability();
    return checkMetalAvailability();
#endif
    return false;
}

bool GraphicsDeviceFactory::isOpenGLAvailable() {
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (!currentWindow) {
        return false;
    }
    
    // Check OpenGL version
    const char* version = (const char*)glGetString(GL_VERSION);
    if (!version) {
        return false;
    }
    
    // We need at least OpenGL 3.3
    int major, minor;
    if (sscanf(version, "%d.%d", &major, &minor) != 2) {
        return false;
    }
    
    return (major > 3) || (major == 3 && minor >= 3);
}

// GraphicsCapabilities Implementation
std::vector<GraphicsCapabilities::DeviceInfo> GraphicsCapabilities::getAvailableDevices() {
    std::vector<DeviceInfo> devices;
    
    // Check Metal availability
#ifdef METAL_AVAILABLE
    if (GraphicsDeviceFactory::isMetalAvailable()) {
        // Forward declare Metal device info function (implemented in .mm file)
        extern GraphicsCapabilities::DeviceInfo getMetalDeviceInfo();
        GraphicsCapabilities::DeviceInfo metalInfo = getMetalDeviceInfo();
        devices.push_back(metalInfo);
    }
#endif
    
    // Check OpenGL availability
    if (GraphicsDeviceFactory::isOpenGLAvailable()) {
        DeviceInfo openglInfo = {};
        openglInfo.api = GraphicsAPIType::OpenGL;
        
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* version = (const char*)glGetString(GL_VERSION);
        
        openglInfo.deviceName = renderer ? std::string(renderer) : "Unknown OpenGL Device";
        openglInfo.driverVersion = version ? std::string(version) : "Unknown";
        openglInfo.supported = true;
        
        GLint maxTextureSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        openglInfo.maxTextureSize = maxTextureSize;
        
        GLint maxColorAttachments;
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
        openglInfo.maxFramebufferAttachments = maxColorAttachments;
        
        // Check for compute shader support (OpenGL 4.3+)
        const char* versionStr = (const char*)glGetString(GL_VERSION);
        int major, minor;
        if (versionStr && sscanf(versionStr, "%d.%d", &major, &minor) == 2) {
            openglInfo.supportsCompute = (major > 4) || (major == 4 && minor >= 3);
        }
        
        // Check for half float support
        openglInfo.supports16BitFloat = true; // Assume support for modern OpenGL
        openglInfo.supports32BitFloat = true;
        
        devices.push_back(openglInfo);
    }
    
    return devices;
}

GraphicsCapabilities::DeviceInfo GraphicsCapabilities::getBestDevice() {
    auto devices = getAvailableDevices();
    
    if (devices.empty()) {
        return {}; // Return default-constructed DeviceInfo
    }
    
    // Prefer Metal on Apple platforms for better performance
    for (const auto& device : devices) {
        if (device.api == GraphicsAPIType::Metal && device.supported) {
            return device;
        }
    }
    
    // Fallback to first available device (likely OpenGL)
    return devices[0];
}

bool GraphicsCapabilities::isHighPerformanceGPUAvailable() {
#ifdef METAL_AVAILABLE
    // Forward declare Metal high-performance GPU check function (implemented in .mm file)
    extern bool isMetalHighPerformanceGPU();
    if (GraphicsDeviceFactory::isMetalAvailable()) {
        return isMetalHighPerformanceGPU();
    }
#endif
    
    // For OpenGL, check renderer string for indicators of high-performance GPU
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    if (renderer) {
        std::string rendererStr(renderer);
        
        // Look for indicators of discrete/high-performance GPUs
        if (rendererStr.find("AMD") != std::string::npos ||
            rendererStr.find("NVIDIA") != std::string::npos ||
            rendererStr.find("Radeon") != std::string::npos ||
            rendererStr.find("GeForce") != std::string::npos) {
            return true;
        }
    }
    
    return false;
} 