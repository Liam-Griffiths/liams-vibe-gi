#ifndef ADAPTIVE_RADIANCE_CASCADES_H
#define ADAPTIVE_RADIANCE_CASCADES_H

#include "GraphicsAPI.h"
#include "RadianceCascades.h"
#ifdef METAL_AVAILABLE
#include "MetalRadianceCascades.h"
#endif

#include <memory>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

/**
 * AdaptiveRadianceCascades - Unified Radiance Cascades Interface
 * 
 * This class provides a unified interface that automatically selects the best
 * available graphics API implementation (Metal or OpenGL) based on platform
 * capabilities and user preferences. It seamlessly handles the differences
 * between the two implementations while exposing a consistent API.
 * 
 * Features:
 * - Automatic API detection and selection
 * - Runtime switching between implementations
 * - Consistent performance across different graphics APIs
 * - Backward compatibility with existing OpenGL code
 * - Performance optimizations specific to each API
 */
class AdaptiveRadianceCascades {
public:
    /**
     * Constructor - Initialize adaptive radiance cascades system
     * 
     * @param window      GLFW window (needed for context creation)
     * @param width       Screen width in pixels
     * @param height      Screen height in pixels
     * @param numCascades Number of cascade levels
     * @param baseSpacing Base spacing for probe placement
     * @param angularBase Angular resolution for directional sampling
     * @param preferMetal Whether to prefer Metal over OpenGL when available
     */
    AdaptiveRadianceCascades(GLFWwindow* window, int width, int height, int numCascades, 
                           float baseSpacing = 1.0f, float angularBase = 360.0f, 
                           bool preferMetal = true);
    
    /**
     * Destructor - Clean up resources
     */
    ~AdaptiveRadianceCascades();

    // Core GI Computation Methods (unified interface)
    
    /**
     * Compute radiance cascades using the selected implementation
     */
    void compute(const glm::mat4& view, const glm::mat4& projection, int activeCascades = -1);
    
    /**
     * Apply temporal filtering
     */
    void applyTemporalFiltering(int activeCascades = -1);
    
    /**
     * Apply blur for noise reduction
     */
    void blur(int activeCascades = -1);
    
    // Screen Space Effects
    
    /**
     * Compute Screen Space Ambient Occlusion
     */
    void computeSSAO(const glm::mat4& projection);
    
    /**
     * Compute Screen Space Reflections
     */
    void computeSSR(unsigned int colorTexture, const glm::mat4& view, 
                    const glm::mat4& projection, const glm::vec3& viewPos);
    
    /**
     * Apply Temporal Anti-Aliasing
     */
    void applyTAA(unsigned int currentFrame, const glm::mat4& currentViewProj, 
                  const glm::mat4& previousViewProj);
    
    // System Management
    
    /**
     * Resize all resources when window size changes
     */
    void resize(int width, int height);
    
    /**
     * Reset temporal accumulation
     */
    void resetTemporalAccumulation();
    
    /**
     * Enable/disable temporal accumulation
     */
    void setTemporalAccumulation(bool enabled);
    
    // G-Buffer Management
    
    /**
     * Bind G-buffer for writing geometry data
     */
    void bindGBufferForWriting();
    
    // Resource Access (returns OpenGL handles for compatibility)
    
    /**
     * Get cascade texture handle (for OpenGL compatibility)
     */
    unsigned int getTexture(int cascade) const;
    
    /**
     * Get cascade framebuffer handle (for OpenGL compatibility)
     */
    unsigned int getCascadeFBO(int cascade) const;
    
    // G-Buffer texture access (OpenGL compatibility)
    unsigned int getGPosition() const;
    unsigned int getGNormal() const;
    unsigned int getGAlbedo() const;
    unsigned int getGVelocity() const;
    unsigned int getGEmission() const;
    unsigned int getHistoryTexture() const;
    
    // Effect texture access
    unsigned int getSSAOTexture() const;
    unsigned int getSSAOBlurTexture() const;
    unsigned int getSSRTexture() const;
    unsigned int getTAATexture() const;
    unsigned int getMergedCascadeTexture() const;
    
    // Utility Methods
    
    /**
     * Get cascade dimensions
     */
    int getCascadeWidth(int cascade) const;
    int getCascadeHeight(int cascade) const;
    
    /**
     * Get the currently active graphics API
     */
    GraphicsAPIType getActiveAPI() const;
    
    /**
     * Get device information
     */
    std::string getDeviceName() const;
    
    /**
     * Check if Metal is being used
     */
    bool isUsingMetal() const;
    
    /**
     * Check if OpenGL is being used
     */
    bool isUsingOpenGL() const;
    
    /**
     * Get performance metrics specific to the active API
     */
    struct PerformanceMetrics {
        GraphicsAPIType api;
        float cascadeComputeTime;
        float blurTime;
        float ssaoTime;
        float memoryUsageMB;
        int frameRate;
        bool usingUnifiedMemory; // Metal-specific
        bool usingTileOptimizations; // Metal-specific
    };
    PerformanceMetrics getPerformanceMetrics() const;
    
    /**
     * Force switch to a specific API (if available)
     * Note: This will recreate all resources
     */
    bool switchToAPI(GraphicsAPIType api);

private:
    // Graphics device and implementation selection
    std::shared_ptr<IGraphicsDevice> graphicsDevice;
    GraphicsAPIType activeAPI;
    GLFWwindow* window;
    
    // Implementation instances
    std::unique_ptr<RadianceCascades> openglImpl;
#ifdef METAL_AVAILABLE
    std::unique_ptr<MetalRadianceCascades> metalImpl;
#endif
    
    // Current configuration
    int screenWidth, screenHeight;
    int numCascades;
    float probeSpacing;
    float angularResolution;
    bool preferMetal;
    
    // Performance tracking
    mutable PerformanceMetrics lastMetrics;
    
    // Private methods
    
    /**
     * Initialize the best available implementation
     */
    bool initializeBestImplementation();
    
    /**
     * Initialize OpenGL implementation
     */
    bool initializeOpenGL();
    
#ifdef METAL_AVAILABLE
    /**
     * Initialize Metal implementation
     */
    bool initializeMetal();
#endif
    
    /**
     * Create appropriate graphics device
     */
    std::shared_ptr<IGraphicsDevice> createGraphicsDevice(GraphicsAPIType preferredAPI);
    
    /**
     * Cleanup current implementation
     */
    void cleanup();
    
    /**
     * Convert Metal texture to OpenGL handle for compatibility
     * (This creates a compatibility layer for existing code)
     */
    unsigned int metalTextureToOpenGLHandle(std::shared_ptr<ITexture> texture) const;
    
    /**
     * Update performance metrics
     */
    void updatePerformanceMetrics() const;
    
    /**
     * Validate API switch capability
     */
    bool canSwitchToAPI(GraphicsAPIType api) const;
};

/**
 * Factory function for easy creation
 * Creates an AdaptiveRadianceCascades instance with optimal settings
 */
std::unique_ptr<AdaptiveRadianceCascades> createOptimalRadianceCascades(
    GLFWwindow* window, int width, int height, int qualityLevel = 2);

#endif // ADAPTIVE_RADIANCE_CASCADES_H 