#include "../include/AdaptiveRadianceCascades.h"
#include "../include/GraphicsAPI.h"
#include <iostream>

AdaptiveRadianceCascades::AdaptiveRadianceCascades(GLFWwindow* window, int width, int height, int numCascades, 
                                                   float baseSpacing, float angularBase, bool preferMetal)
    : window(window)
    , screenWidth(width)
    , screenHeight(height)
    , numCascades(numCascades)
    , probeSpacing(baseSpacing)
    , angularResolution(angularBase)
    , preferMetal(preferMetal)
    , activeAPI(GraphicsAPIType::OpenGL)
    , openglImpl(nullptr)
#ifdef METAL_AVAILABLE
    , metalImpl(nullptr)
#endif
{
    std::cout << "Initializing Adaptive Radiance Cascades System..." << std::endl;
    std::cout << "Target resolution: " << width << "x" << height << std::endl;
    std::cout << "Number of cascades: " << numCascades << std::endl;
    std::cout << "Prefer Metal: " << (preferMetal ? "Yes" : "No") << std::endl;
    
    initializeBestImplementation();
}

AdaptiveRadianceCascades::~AdaptiveRadianceCascades() {
    cleanup();
}

bool AdaptiveRadianceCascades::initializeBestImplementation() {
    // Determine the best graphics API to use
    GraphicsAPIType preferredAPI = preferMetal ? GraphicsAPIType::Metal : GraphicsAPIType::OpenGL;
    
    // Create graphics device factory and get the best available device
    std::shared_ptr<IGraphicsDevice> device = GraphicsDeviceFactory::createDevice(preferredAPI);
    
    if (!device) {
        std::cerr << "Failed to create graphics device!" << std::endl;
        return false;
    }
    
    activeAPI = device->getAPIType();
    graphicsDevice = device;
    
    std::cout << "Selected Graphics API: " << 
        (activeAPI == GraphicsAPIType::Metal ? "Metal" : "OpenGL") << std::endl;
    std::cout << "Device: " << device->getDeviceName() << std::endl;
    
    // Initialize the appropriate radiance cascades implementation
    bool success = false;
    if (activeAPI == GraphicsAPIType::Metal) {
        success = initializeMetal();
    } else {
        success = initializeOpenGL();
    }
    
    if (success) {
        std::cout << "Adaptive Radiance Cascades initialization complete!" << std::endl;
    }
    return success;
}

bool AdaptiveRadianceCascades::initializeOpenGL() {
    std::cout << "Initializing OpenGL Radiance Cascades..." << std::endl;
    
    try {
        openglImpl = std::make_unique<RadianceCascades>(
            screenWidth, screenHeight, numCascades, probeSpacing, angularResolution);
        std::cout << "OpenGL Radiance Cascades initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OpenGL Radiance Cascades: " << e.what() << std::endl;
        openglImpl = nullptr;
        return false;
    }
}

#ifdef METAL_AVAILABLE
bool AdaptiveRadianceCascades::initializeMetal() {
    std::cout << "Initializing Metal Radiance Cascades..." << std::endl;
    
    try {
        metalImpl = std::make_unique<MetalRadianceCascades>(
            graphicsDevice, screenWidth, screenHeight, numCascades, probeSpacing, angularResolution);
        std::cout << "Metal Radiance Cascades initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Metal Radiance Cascades: " << e.what() << std::endl;
        std::cerr << "Falling back to OpenGL implementation..." << std::endl;
        
        // Fallback to OpenGL if Metal fails
        activeAPI = GraphicsAPIType::OpenGL;
        metalImpl = nullptr;
        return initializeOpenGL();
    }
}
#else
bool AdaptiveRadianceCascades::initializeMetal() {
    std::cout << "Metal not available, falling back to OpenGL..." << std::endl;
    activeAPI = GraphicsAPIType::OpenGL;
    return initializeOpenGL();
}
#endif

void AdaptiveRadianceCascades::compute(const glm::mat4& view, const glm::mat4& projection, int activeCascades) {
    if (activeAPI == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalImpl) {
            metalImpl->compute(view, projection, activeCascades);
        }
#endif
    } else {
        if (openglImpl) {
            openglImpl->compute(view, projection, activeCascades);
        }
    }
}

void AdaptiveRadianceCascades::applyTemporalFiltering(int activeCascades) {
    if (activeAPI == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalImpl) {
            metalImpl->applyTemporalFiltering(activeCascades);
        }
#endif
    } else {
        if (openglImpl) {
            openglImpl->applyTemporalFiltering(activeCascades);
        }
    }
}

void AdaptiveRadianceCascades::blur(int activeCascades) {
    if (activeAPI == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalImpl) {
            metalImpl->blur(activeCascades);
        }
#endif
    } else {
        if (openglImpl) {
            openglImpl->blur(activeCascades);
        }
    }
}

void AdaptiveRadianceCascades::computeSSAO(const glm::mat4& projection) {
    if (activeAPI == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalImpl) {
            metalImpl->computeSSAO(projection);
        }
#endif
    } else {
        if (openglImpl) {
            openglImpl->computeSSAO(projection);
        }
    }
}

void AdaptiveRadianceCascades::computeSSR(std::shared_ptr<ITexture> colorTexture, const glm::mat4& view, 
                                          const glm::mat4& projection, const glm::vec3& viewPos) {
    if (activeAPI == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalImpl) {
            metalImpl->computeSSR(colorTexture, view, projection, viewPos);
        }
#endif
    } else {
        if (openglImpl) {
            // For OpenGL, we need to convert the colorTexture to a raw handle
            // This is a simplification - in practice you'd need proper texture conversion
            openglImpl->computeSSR(colorTexture->getHandle(), view, projection, viewPos);
        }
    }
}

void AdaptiveRadianceCascades::applyTAA(std::shared_ptr<ITexture> currentFrame, 
                                        const glm::mat4& currentViewProj, const glm::mat4& previousViewProj) {
    if (!isInitialized) return;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            metalRadianceCascades->applyTAA(currentFrame, currentViewProj, previousViewProj);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            // For OpenGL, convert texture handle
            openglRadianceCascades->applyTAA(currentFrame->getHandle(), currentViewProj, previousViewProj);
        }
    }
}

void AdaptiveRadianceCascades::resize(int width, int height) {
    if (width == screenWidth && height == screenHeight) return;
    
    screenWidth = width;
    screenHeight = height;
    
    std::cout << "Resizing Adaptive Radiance Cascades to " << width << "x" << height << std::endl;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            metalRadianceCascades->resize(width, height);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            openglRadianceCascades->resize(width, height);
        }
    }
}

void AdaptiveRadianceCascades::resetTemporalAccumulation() {
    if (!isInitialized) return;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            metalRadianceCascades->resetTemporalAccumulation();
        }
#endif
    } else {
        if (openglRadianceCascades) {
            openglRadianceCascades->resetTemporalAccumulation();
        }
    }
}

void AdaptiveRadianceCascades::setTemporalAccumulation(bool enabled) {
    if (!isInitialized) return;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            metalRadianceCascades->setTemporalAccumulation(enabled);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            openglRadianceCascades->setTemporalAccumulation(enabled);
        }
    }
}

void AdaptiveRadianceCascades::bindGBufferForWriting() {
    if (!isInitialized) return;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            metalRadianceCascades->bindGBufferForWriting();
        }
#endif
    } else {
        if (openglRadianceCascades) {
            openglRadianceCascades->bindGBufferForWriting();
        }
    }
}

std::shared_ptr<ITexture> AdaptiveRadianceCascades::getCascadeTexture(int cascade) const {
    if (!isInitialized) return nullptr;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            return metalRadianceCascades->getCascadeTexture(cascade);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            // For OpenGL, we'd need to wrap the texture handle in our abstraction
            // This is a simplified implementation
            return nullptr; // TODO: Implement OpenGL texture wrapping
        }
    }
    
    return nullptr;
}

std::shared_ptr<IFramebuffer> AdaptiveRadianceCascades::getCascadeFramebuffer(int cascade) const {
    if (!isInitialized) return nullptr;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            return metalRadianceCascades->getCascadeFramebuffer(cascade);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            // For OpenGL, we'd need to wrap the framebuffer handle
            return nullptr; // TODO: Implement OpenGL framebuffer wrapping
        }
    }
    
    return nullptr;
}

GraphicsAPIType AdaptiveRadianceCascades::getCurrentAPIType() const {
    return currentAPIType;
}

std::string AdaptiveRadianceCascades::getCurrentAPIName() const {
    switch (currentAPIType) {
        case GraphicsAPIType::Metal:
            return "Metal";
        case GraphicsAPIType::OpenGL:
            return "OpenGL";
        default:
            return "Unknown";
    }
}

std::shared_ptr<IGraphicsDevice> AdaptiveRadianceCascades::getGraphicsDevice() const {
    return graphicsDevice;
}

bool AdaptiveRadianceCascades::isUsingMetal() const {
    return currentAPIType == GraphicsAPIType::Metal;
}

bool AdaptiveRadianceCascades::isUsingOpenGL() const {
    return currentAPIType == GraphicsAPIType::OpenGL;
}

bool AdaptiveRadianceCascades::canSwitchAPI() const {
    // API switching would require recreating all resources
    // For now, we only support selection at initialization
    return false;
}

void AdaptiveRadianceCascades::getPerformanceStats(float& frameTime, int& activeCascades, 
                                                   std::string& apiInfo) const {
    if (!isInitialized) {
        frameTime = 0.0f;
        activeCascades = 0;
        apiInfo = "Not Initialized";
        return;
    }
    
    // Get basic stats
    activeCascades = numCascades;
    apiInfo = getCurrentAPIName();
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades && metalRadianceCascades->isMetalOptimized()) {
            apiInfo += " (Optimized)";
        }
#endif
    }
    
    // Frame time would need to be measured by the calling code
    frameTime = 0.0f; // TODO: Implement performance timing
}

int AdaptiveRadianceCascades::getCascadeWidth(int cascade) const {
    if (!isInitialized) return 0;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            return metalRadianceCascades->getCascadeWidth(cascade);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            return openglRadianceCascades->getCascadeWidth(cascade);
        }
    }
    
    return 0;
}

int AdaptiveRadianceCascades::getCascadeHeight(int cascade) const {
    if (!isInitialized) return 0;
    
    if (currentAPIType == GraphicsAPIType::Metal) {
#ifdef METAL_AVAILABLE
        if (metalRadianceCascades) {
            return metalRadianceCascades->getCascadeHeight(cascade);
        }
#endif
    } else {
        if (openglRadianceCascades) {
            return openglRadianceCascades->getCascadeHeight(cascade);
        }
    }
    
    return 0;
}

void AdaptiveRadianceCascades::cleanup() {
    std::cout << "Cleaning up Adaptive Radiance Cascades..." << std::endl;
    
#ifdef METAL_AVAILABLE
    metalRadianceCascades.reset();
#endif
    openglRadianceCascades.reset();
    graphicsDevice.reset();
    
    isInitialized = false;
    std::cout << "Adaptive Radiance Cascades cleanup complete" << std::endl;
} 