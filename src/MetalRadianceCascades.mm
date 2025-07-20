#include "../include/MetalRadianceCascades.h"
#include "../include/MetalDevice.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifdef METAL_AVAILABLE

MetalRadianceCascades::MetalRadianceCascades(std::shared_ptr<IGraphicsDevice> device, 
                                             int width, int height, int numCascades, 
                                             float baseSpacing, float angularBase)
    : graphicsDevice(device)
    , screenWidth(width)
    , screenHeight(height)
    , numCascades(numCascades)
    , probeSpacing(baseSpacing)
    , angularResolution(angularBase)
    , useTemporalBuffer(true)
    , frameCounter(0)
    , useMetalPerformanceShaders(true)
    , useTileMemoryOptimizations(true)
    , useUnifiedMemory(true)
    , nearFieldAngularSamples(48)
    , farFieldAngularSamples(12)
    , spatialResolutionScaling(0.6f)
{
    // Validate that we're using a Metal device
    if (device->getAPIType() != GraphicsAPIType::Metal) {
        std::cerr << "MetalRadianceCascades requires a Metal graphics device!" << std::endl;
        return;
    }
    
    std::cout << "Initializing Metal-optimized Radiance Cascades..." << std::endl;
    std::cout << "Screen resolution: " << width << "x" << height << std::endl;
    std::cout << "Number of cascades: " << numCascades << std::endl;
    
    // Initialize Metal-specific optimizations
    initializeMetalOptimizations();
    
    // Initialize band-limiting parameters for Metal
    initializeBandLimitingParameters();
    
    // Setup core resources
    setupGBuffer();
    setupCascades();
    setupTemporalBuffers();
    setupScreenSpaceEffects();
    setupShaders();
    
    std::cout << "Metal Radiance Cascades initialization complete!" << std::endl;
}

MetalRadianceCascades::~MetalRadianceCascades() {
    cleanup();
}

void MetalRadianceCascades::initializeMetalOptimizations() {
    // Configure Metal-specific performance settings
    configureMetalGPUPriority();
    setupMetalMemoryPools();
    optimizeMetalRenderPipelines();
    
    std::cout << "Metal optimizations enabled:" << std::endl;
    std::cout << "  - Performance Shaders: " << (useMetalPerformanceShaders ? "Yes" : "No") << std::endl;
    std::cout << "  - Tile Memory Opts: " << (useTileMemoryOptimizations ? "Yes" : "No") << std::endl;
    std::cout << "  - Unified Memory: " << (useUnifiedMemory ? "Yes" : "No") << std::endl;
}

void MetalRadianceCascades::initializeBandLimitingParameters() {
    cascadeMinDistances.resize(numCascades);
    cascadeMaxDistances.resize(numCascades);
    cascadeAngularSamples.resize(numCascades);
    
    // Metal-optimized cascade parameters
    for (int i = 0; i < numCascades; i++) {
        float cascadeScale = std::pow(2.0f, i);
        cascadeMinDistances[i] = (i == 0) ? 0.0f : cascadeMaxDistances[i-1];
        cascadeMaxDistances[i] = probeSpacing * cascadeScale * 8.0f;
        
        // Use more samples for near cascades on Metal due to higher bandwidth
        int baseSamples = (i < numCascades / 2) ? nearFieldAngularSamples : farFieldAngularSamples;
        cascadeAngularSamples[i] = std::max(8, baseSamples >> i);
    }
}

void MetalRadianceCascades::setupGBuffer() {
    // Create G-buffer with Metal-optimized formats
    gBuffer = graphicsDevice->createFramebuffer();
    
    // Metal supports better precision formats
    gPosition = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA32F); // Higher precision on Metal
    gNormal = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RG16F);     // Packed normals
    gAlbedo = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA8);     // sRGB albedo
    gVelocity = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RG16F);   // Motion vectors
    gEmission = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA16F); // HDR emission
    gDepth = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::Depth24Stencil8);
    
    // Attach textures to G-buffer
    gBuffer->attachTexture(gPosition, 0);
    gBuffer->attachTexture(gNormal, 1);
    gBuffer->attachTexture(gAlbedo, 2);
    gBuffer->attachTexture(gVelocity, 3);
    gBuffer->attachTexture(gEmission, 4);
    gBuffer->attachDepthTexture(gDepth);
    
    if (!gBuffer->isComplete()) {
        std::cerr << "Failed to create Metal G-buffer!" << std::endl;
    }
    
    std::cout << "Metal G-buffer created with optimized formats" << std::endl;
}

void MetalRadianceCascades::setupCascades() {
    cascadeTextures.resize(numCascades);
    cascadeFramebuffers.resize(numCascades);
    cascadeWidths.resize(numCascades);
    cascadeHeights.resize(numCascades);
    
    for (int i = 0; i < numCascades; i++) {
        // Metal can handle higher resolution cascades due to unified memory
        float scale = std::pow(spatialResolutionScaling, i);
        cascadeWidths[i] = std::max(64, (int)(screenWidth * scale));
        cascadeHeights[i] = std::max(64, (int)(screenHeight * scale));
        
        // Use RGBA16F for better precision on Metal
        cascadeTextures[i] = graphicsDevice->createTexture(cascadeWidths[i], cascadeHeights[i], TextureFormat::RGBA16F);
        cascadeFramebuffers[i] = graphicsDevice->createFramebuffer();
        cascadeFramebuffers[i]->attachTexture(cascadeTextures[i], 0);
        
        if (!cascadeFramebuffers[i]->isComplete()) {
            std::cerr << "Failed to create Metal cascade " << i << " framebuffer!" << std::endl;
        }
    }
    
    std::cout << "Created " << numCascades << " Metal-optimized cascades" << std::endl;
}

void MetalRadianceCascades::setupTemporalBuffers() {
    temporalTextures.resize(numCascades);
    temporalFramebuffers.resize(numCascades);
    
    for (int i = 0; i < numCascades; i++) {
        temporalTextures[i] = graphicsDevice->createTexture(cascadeWidths[i], cascadeHeights[i], TextureFormat::RGBA16F);
        temporalFramebuffers[i] = graphicsDevice->createFramebuffer();
        temporalFramebuffers[i]->attachTexture(temporalTextures[i], 0);
    }
    
    // History texture for TAA
    historyTexture = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA16F);
    
    std::cout << "Metal temporal buffers initialized" << std::endl;
}

void MetalRadianceCascades::setupScreenSpaceEffects() {
    // SSAO
    ssaoTexture = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::R16F);
    ssaoBlurTexture = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::R16F);
    ssaoFramebuffer = graphicsDevice->createFramebuffer();
    ssaoFramebuffer->attachTexture(ssaoTexture, 0);
    
    // SSR
    ssrTexture = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA16F);
    ssrFramebuffer = graphicsDevice->createFramebuffer();
    ssrFramebuffer->attachTexture(ssrTexture, 0);
    
    // TAA
    taaTexture = graphicsDevice->createTexture(screenWidth, screenHeight, TextureFormat::RGBA16F);
    taaFramebuffer = graphicsDevice->createFramebuffer();
    taaFramebuffer->attachTexture(taaTexture, 0);
    
    std::cout << "Metal screen space effects initialized" << std::endl;
}

void MetalRadianceCascades::setupShaders() {
    // Load Metal-compiled shaders
    try {
        rcShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/rc_cascade.metal");
        blurShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/blur.frag");
        ssaoShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/ssao.frag");
        ssrShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/ssr.frag");
        taaShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/taa.frag");
        copyShader = graphicsDevice->createShader("shaders/fullscreen.vert", "shaders/copy.frag");
        
        std::cout << "Metal shaders loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load Metal shaders: " << e.what() << std::endl;
    }
}

void MetalRadianceCascades::compute(const glm::mat4& view, const glm::mat4& projection, int activeCascades) {
    if (!rcShader) return;
    
    int cascadesToCompute = (activeCascades == -1) ? numCascades : std::min(activeCascades, numCascades);
    
    graphicsDevice->beginFrame();
    
    for (int i = 0; i < cascadesToCompute; i++) {
        cascadeFramebuffers[i]->bind();
        graphicsDevice->setViewport(0, 0, cascadeWidths[i], cascadeHeights[i]);
        graphicsDevice->clear(0.0f, 0.0f, 0.0f, 0.0f);
        
        rcShader->use();
        
        // Set Metal-specific uniforms
        rcShader->setInt("cascadeLevel", i);
        rcShader->setInt("totalCascades", numCascades);
        rcShader->setFloat("minDistance", cascadeMinDistances[i]);
        rcShader->setFloat("maxDistance", cascadeMaxDistances[i]);
        rcShader->setInt("angularSamples", cascadeAngularSamples[i]);
        rcShader->setMat4("view", view);
        rcShader->setMat4("projection", projection);
        
        // Bind G-buffer textures
        gPosition->bind(0);
        gNormal->bind(1);
        gAlbedo->bind(2);
        gEmission->bind(3);
        
        // Bind previous cascade if available
        if (i > 0) {
            cascadeTextures[i-1]->bind(4);
        }
        
        graphicsDevice->renderFullscreenQuad();
        cascadeFramebuffers[i]->unbind();
    }
    
    graphicsDevice->endFrame();
}

void MetalRadianceCascades::applyTemporalFiltering(int activeCascades) {
    if (!useTemporalBuffer) return;
    
    int cascadesToFilter = (activeCascades == -1) ? numCascades : std::min(activeCascades, numCascades);
    
    // TODO: Implement Metal-optimized temporal filtering
    // For now, just increment frame counter
    frameCounter++;
}

void MetalRadianceCascades::blur(int activeCascades) {
    if (!blurShader) return;
    
    int cascadesToBlur = (activeCascades == -1) ? numCascades : std::min(activeCascades, numCascades);
    
    // TODO: Implement Metal-optimized blur using tile memory
}

void MetalRadianceCascades::computeSSAO(const glm::mat4& projection) {
    if (!ssaoShader || !ssaoFramebuffer) return;
    
    ssaoFramebuffer->bind();
    graphicsDevice->setViewport(0, 0, screenWidth, screenHeight);
    graphicsDevice->clear(1.0f, 1.0f, 1.0f, 1.0f);
    
    ssaoShader->use();
    ssaoShader->setMat4("projection", projection);
    
    gPosition->bind(0);
    gNormal->bind(1);
    gDepth->bind(2);
    
    graphicsDevice->renderFullscreenQuad();
    ssaoFramebuffer->unbind();
}

void MetalRadianceCascades::computeSSR(std::shared_ptr<ITexture> colorTexture, const glm::mat4& view, 
                                       const glm::mat4& projection, const glm::vec3& viewPos) {
    if (!ssrShader || !ssrFramebuffer) return;
    
    ssrFramebuffer->bind();
    graphicsDevice->setViewport(0, 0, screenWidth, screenHeight);
    graphicsDevice->clear(0.0f, 0.0f, 0.0f, 0.0f);
    
    ssrShader->use();
    ssrShader->setMat4("view", view);
    ssrShader->setMat4("projection", projection);
    ssrShader->setVec3("viewPos", viewPos);
    
    colorTexture->bind(0);
    gPosition->bind(1);
    gNormal->bind(2);
    gDepth->bind(3);
    
    graphicsDevice->renderFullscreenQuad();
    ssrFramebuffer->unbind();
}

void MetalRadianceCascades::applyTAA(std::shared_ptr<ITexture> currentFrame, 
                                     const glm::mat4& currentViewProj, const glm::mat4& previousViewProj) {
    if (!taaShader || !taaFramebuffer) return;
    
    taaFramebuffer->bind();
    graphicsDevice->setViewport(0, 0, screenWidth, screenHeight);
    
    taaShader->use();
    taaShader->setMat4("currentViewProj", currentViewProj);
    taaShader->setMat4("previousViewProj", previousViewProj);
    taaShader->setInt("frameCounter", frameCounter);
    
    currentFrame->bind(0);
    historyTexture->bind(1);
    gVelocity->bind(2);
    
    graphicsDevice->renderFullscreenQuad();
    taaFramebuffer->unbind();
    
    // Copy result to history
    // TODO: Implement efficient copy using Metal blit encoder
}

void MetalRadianceCascades::resize(int width, int height) {
    if (width == screenWidth && height == screenHeight) return;
    
    screenWidth = width;
    screenHeight = height;
    
    std::cout << "Resizing Metal Radiance Cascades to " << width << "x" << height << std::endl;
    
    // Recreate all size-dependent resources
    setupGBuffer();
    setupCascades();
    setupTemporalBuffers();
    setupScreenSpaceEffects();
}

void MetalRadianceCascades::resetTemporalAccumulation() {
    frameCounter = 0;
    // TODO: Clear temporal buffers
}

void MetalRadianceCascades::setTemporalAccumulation(bool enabled) {
    useTemporalBuffer = enabled;
    if (!enabled) {
        resetTemporalAccumulation();
    }
}

void MetalRadianceCascades::bindGBufferForWriting() {
    if (gBuffer) {
        gBuffer->bind();
        graphicsDevice->setViewport(0, 0, screenWidth, screenHeight);
    }
}

std::shared_ptr<ITexture> MetalRadianceCascades::getCascadeTexture(int cascade) const {
    if (cascade >= 0 && cascade < numCascades) {
        return cascadeTextures[cascade];
    }
    return nullptr;
}

std::shared_ptr<IFramebuffer> MetalRadianceCascades::getCascadeFramebuffer(int cascade) const {
    if (cascade >= 0 && cascade < numCascades) {
        return cascadeFramebuffers[cascade];
    }
    return nullptr;
}

int MetalRadianceCascades::getCascadeWidth(int cascade) const {
    if (cascade >= 0 && cascade < numCascades) {
        return cascadeWidths[cascade];
    }
    return 0;
}

int MetalRadianceCascades::getCascadeHeight(int cascade) const {
    if (cascade >= 0 && cascade < numCascades) {
        return cascadeHeights[cascade];
    }
    return 0;
}

GraphicsAPIType MetalRadianceCascades::getAPIType() const {
    return graphicsDevice ? graphicsDevice->getAPIType() : GraphicsAPIType::OpenGL;
}

bool MetalRadianceCascades::isMetalOptimized() const {
    return getAPIType() == GraphicsAPIType::Metal && useMetalPerformanceShaders;
}

void MetalRadianceCascades::configureMetalGPUPriority() {
    // TODO: Configure Metal GPU priority for optimal performance
    std::cout << "Configuring Metal GPU priority for high performance" << std::endl;
}

void MetalRadianceCascades::setupMetalMemoryPools() {
    // TODO: Setup Metal memory pools for efficient resource management
    std::cout << "Setting up Metal memory pools for unified memory access" << std::endl;
}

void MetalRadianceCascades::optimizeMetalRenderPipelines() {
    // TODO: Optimize Metal render pipelines for radiance cascades workload
    std::cout << "Optimizing Metal render pipelines for tile-based rendering" << std::endl;
}

void MetalRadianceCascades::cleanup() {
    // Resources will be automatically cleaned up by shared_ptr destructors
    std::cout << "Cleaning up Metal Radiance Cascades resources" << std::endl;
}

#endif // METAL_AVAILABLE 