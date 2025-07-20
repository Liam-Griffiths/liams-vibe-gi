#include "../include/GraphicsAPI.h"

#ifdef METAL_AVAILABLE
#import <Metal/Metal.h>

// Check if Metal is available on this system
bool checkMetalAvailability() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return device != nil;
}

// Get Metal device information
GraphicsCapabilities::DeviceInfo getMetalDeviceInfo() {
    GraphicsCapabilities::DeviceInfo metalInfo = {};
    metalInfo.api = GraphicsAPIType::Metal;
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        metalInfo.deviceName = std::string([device.name UTF8String]);
        metalInfo.supported = true;
        metalInfo.maxTextureSize = 16384; // Metal typical limit
        metalInfo.maxFramebufferAttachments = 8;
        metalInfo.supportsCompute = true;
        metalInfo.supports16BitFloat = true;
        metalInfo.supports32BitFloat = true;
        metalInfo.driverVersion = "Metal"; // Could get more specific version
        
        [device release]; // Clean up
    }
    
    return metalInfo;
}

// Check if Metal device is high-performance
bool isMetalHighPerformanceGPU() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        bool isHighPerf = !device.isLowPower;
        [device release]; // Clean up
        return isHighPerf;
    }
    return false;
}

#endif // METAL_AVAILABLE 