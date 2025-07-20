#ifndef METAL_RADIANCE_CASCADES_H
#define METAL_RADIANCE_CASCADES_H

#include "GraphicsAPI.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

/**
 * MetalRadianceCascades - Metal-optimized Radiance Cascades Implementation
 * 
 * This class provides a Metal-based implementation of the radiance cascades
 * global illumination technique, designed to take advantage of Apple's Metal
 * performance shaders and unified memory architecture for improved performance
 * over the OpenGL implementation.
 * 
 * Key Metal optimizations:
 * - Unified memory architecture reduces CPU-GPU transfer overhead
 * - Metal Performance Shaders for optimized compute operations
 * - Tile-based deferred rendering for better bandwidth utilization
 * - Native integration with macOS GPU schedulers
 */
class MetalRadianceCascades {
public:
    /**
     * Constructor - Initialize Metal-based radiance cascades
     * 
     * @param device      Graphics device (must be Metal-based)
     * @param width       Screen width in pixels
     * @param height      Screen height in pixels
     * @param numCascades Number of cascade levels (typically 4-6 for Metal)
     * @param baseSpacing Base spacing for probe placement
     * @param angularBase Angular resolution for directional sampling
     */
    MetalRadianceCascades(std::shared_ptr<IGraphicsDevice> device, 
                         int width, int height, int numCascades, 
                         float baseSpacing = 1.0f, float angularBase = 360.0f);
    
    /**
     * Destructor - Clean up Metal resources
     */
    ~MetalRadianceCascades();

    // Core GI Computation Methods
    
    /**
     * Compute radiance cascades using Metal compute shaders
     * 
     * @param view       Camera view matrix
     * @param projection Camera projection matrix
     * @param activeCascades Number of cascades to compute (-1 for all)
     */
    void compute(const glm::mat4& view, const glm::mat4& projection, int activeCascades = -1);
    
    /**
     * Apply temporal filtering using Metal's optimized memory bandwidth
     * 
     * @param activeCascades Number of cascades to filter (-1 for all)
     */
    void applyTemporalFiltering(int activeCascades = -1);
    
    /**
     * Apply Metal-optimized blur using tile memory
     * 
     * @param activeCascades Number of cascades to blur (-1 for all)
     */
    void blur(int activeCascades = -1);
    
    // Screen Space Effects with Metal optimizations
    
    /**
     * Compute SSAO using Metal's tile-based rendering advantages
     * 
     * @param projection Camera projection matrix
     */
    void computeSSAO(const glm::mat4& projection);
    
    /**
     * Compute SSR using Metal's high-bandwidth memory access
     * 
     * @param colorTexture Current frame color texture
     * @param view Current view matrix
     * @param projection Current projection matrix
     * @param viewPos Camera position in world space
     */
    void computeSSR(std::shared_ptr<ITexture> colorTexture, const glm::mat4& view, 
                    const glm::mat4& projection, const glm::vec3& viewPos);
    
    /**
     * Apply TAA using Metal's unified memory for efficient history access
     * 
     * @param currentFrame Current frame texture
     * @param currentViewProj Current view-projection matrix
     * @param previousViewProj Previous frame's view-projection matrix
     */
    void applyTAA(std::shared_ptr<ITexture> currentFrame, 
                  const glm::mat4& currentViewProj, const glm::mat4& previousViewProj);
    
    // System Management
    
    /**
     * Resize all Metal resources when window size changes
     * 
     * @param width  New screen width
     * @param height New screen height
     */
    void resize(int width, int height);
    
    /**
     * Reset temporal accumulation buffers
     */
    void resetTemporalAccumulation();
    
    /**
     * Enable or disable temporal accumulation
     */
    void setTemporalAccumulation(bool enabled);
    
    // G-Buffer Management
    
    /**
     * Bind G-buffer for writing geometry data using Metal render pass
     */
    void bindGBufferForWriting();
    
    /**
     * Setup G-buffer with Metal-optimized formats
     */
    void setupGBuffer();
    
    // Resource Access Methods
    
    /**
     * Get radiance cascade texture for a specific level
     */
    std::shared_ptr<ITexture> getCascadeTexture(int cascade) const;
    
    /**
     * Get cascade framebuffer for a specific level
     */
    std::shared_ptr<IFramebuffer> getCascadeFramebuffer(int cascade) const;
    
    // G-Buffer texture access
    std::shared_ptr<ITexture> getGPosition() const { return gPosition; }
    std::shared_ptr<ITexture> getGNormal() const { return gNormal; }
    std::shared_ptr<ITexture> getGAlbedo() const { return gAlbedo; }
    std::shared_ptr<ITexture> getGVelocity() const { return gVelocity; }
    std::shared_ptr<ITexture> getGEmission() const { return gEmission; }
    std::shared_ptr<ITexture> getHistoryTexture() const { return historyTexture; }
    
    // Effect texture access
    std::shared_ptr<ITexture> getSSAOTexture() const { return ssaoTexture; }
    std::shared_ptr<ITexture> getSSRTexture() const { return ssrTexture; }
    std::shared_ptr<ITexture> getTAATexture() const { return taaTexture; }
    
    // Utility Methods
    
    /**
     * Get width/height of a specific cascade level
     */
    int getCascadeWidth(int cascade) const;
    int getCascadeHeight(int cascade) const;
    
    /**
     * Get device API type for validation
     */
    GraphicsAPIType getAPIType() const;
    
    /**
     * Check if Metal is being used and properly initialized
     */
    bool isMetalOptimized() const;

private:
    // Core properties
    std::shared_ptr<IGraphicsDevice> graphicsDevice;
    int screenWidth, screenHeight;
    int numCascades;
    
    // Metal-specific optimizations
    bool useMetalPerformanceShaders;
    bool useTileMemoryOptimizations;
    bool useUnifiedMemory;
    
    // Cascade resources
    std::vector<std::shared_ptr<ITexture>> cascadeTextures;
    std::vector<std::shared_ptr<IFramebuffer>> cascadeFramebuffers;
    std::vector<int> cascadeWidths;
    std::vector<int> cascadeHeights;
    
    // Temporal accumulation
    std::vector<std::shared_ptr<ITexture>> temporalTextures;
    std::vector<std::shared_ptr<IFramebuffer>> temporalFramebuffers;
    bool useTemporalBuffer;
    int frameCounter;
    
    // G-Buffer resources (using Metal-optimized formats)
    std::shared_ptr<IFramebuffer> gBuffer;
    std::shared_ptr<ITexture> gPosition;     // RGB10A2 for better precision on Metal
    std::shared_ptr<ITexture> gNormal;       // RG16F packed normals
    std::shared_ptr<ITexture> gAlbedo;       // RGBA8 sRGB
    std::shared_ptr<ITexture> gVelocity;     // RG16F motion vectors
    std::shared_ptr<ITexture> gEmission;     // RGB9E5 for HDR emission
    std::shared_ptr<ITexture> gDepth;        // Depth32F for precision
    std::shared_ptr<ITexture> historyTexture; // RGBA16F for TAA
    
    // Screen space effects
    std::shared_ptr<ITexture> ssaoTexture;
    std::shared_ptr<ITexture> ssaoBlurTexture;
    std::shared_ptr<ITexture> ssrTexture;
    std::shared_ptr<ITexture> taaTexture;
    
    std::shared_ptr<IFramebuffer> ssaoFramebuffer;
    std::shared_ptr<IFramebuffer> ssrFramebuffer;
    std::shared_ptr<IFramebuffer> taaFramebuffer;
    
    // Shaders (Metal-compiled)
    std::shared_ptr<IShader> rcShader;
    std::shared_ptr<IShader> blurShader;
    std::shared_ptr<IShader> ssaoShader;
    std::shared_ptr<IShader> ssrShader;
    std::shared_ptr<IShader> taaShader;
    std::shared_ptr<IShader> copyShader;
    
    // Cascade parameters optimized for Metal
    float probeSpacing;
    float angularResolution;
    std::vector<float> cascadeMinDistances;
    std::vector<float> cascadeMaxDistances;
    std::vector<int> cascadeAngularSamples;
    
    // Metal-specific performance parameters
    int nearFieldAngularSamples;
    int farFieldAngularSamples;
    float spatialResolutionScaling;
    
    // Private setup methods
    void initializeMetalOptimizations();
    void setupCascades();
    void setupTemporalBuffers();
    void setupScreenSpaceEffects();
    void setupShaders();
    void initializeBandLimitingParameters();
    
    // Metal-specific optimization methods
    void configureMetalGPUPriority();
    void setupMetalMemoryPools();
    void optimizeMetalRenderPipelines();
    
    // Resource cleanup
    void cleanup();
};

#endif // METAL_RADIANCE_CASCADES_H 