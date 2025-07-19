/**
 * RadianceCascades.h - Real-time Global Illumination using Radiance Cascades
 * 
 * This file implements the Radiance Cascades algorithm, a cutting-edge technique for
 * real-time global illumination. Radiance cascades provide accurate multi-bounce
 * indirect lighting by storing radiance information at multiple spatial scales.
 * 
 * The algorithm works by:
 * 1. Generating a G-buffer with geometry information
 * 2. Computing radiance at multiple cascade levels (different spatial resolutions)
 * 3. Propagating light through the cascades to simulate light bouncing
 * 4. Applying temporal filtering for stability
 * 5. Combining with direct lighting for final illumination
 * 
 * Additional features:
 * - Screen Space Ambient Occlusion (SSAO) for contact shadows
 * - Temporal Anti-Aliasing (TAA) for temporal upsampling
 * - Motion vectors for temporal effects
 * 
 * References:
 * - "Radiance Cascades: A Novel Approach to Real-Time Global Illumination"
 * - Screen-space techniques in real-time rendering
 */

#ifndef RADIANCE_CASCADES_H
#define RADIANCE_CASCADES_H

#include <vector>
#include "../include/Shader.h"
#include <glm/glm.hpp>

/**
 * RadianceCascades class - Advanced Global Illumination System
 * 
 * Implements real-time global illumination using the radiance cascades technique.
 * This provides accurate indirect lighting with multiple light bounces at
 * interactive frame rates.
 */
class RadianceCascades {
public:
    /**
     * Constructor - Initialize the radiance cascades system
     * 
     * @param width        Screen width in pixels
     * @param height       Screen height in pixels  
     * @param numCascades  Number of cascade levels (typically 4-8)
     * @param baseSpacing  Base spacing for probe placement
     * @param angularBase  Angular resolution for directional sampling
     */
    RadianceCascades(int width, int height, int numCascades, float baseSpacing = 1.0f, float angularBase = 360.0f);
    
    /**
     * Destructor - Clean up OpenGL resources
     */
    ~RadianceCascades();

    // Core GI Computation Methods
    
    /**
     * Compute radiance cascades for global illumination
     * This is the main GI computation that propagates light through multiple scales
     * 
     * @param shader     Radiance cascade compute shader
     * @param view       Camera view matrix
     * @param projection Camera projection matrix
     */
    void compute(Shader& shader, const glm::mat4& view, const glm::mat4& projection, int activeCascades = -1);
    
    /**
     * Apply temporal blur to smooth GI and reduce noise
     * Uses separable bilateral filtering for quality and performance
     * 
     * @param blurShader  Shader for blur computation
     * @param activeCascades Number of cascades to blur (-1 for all)
     */
    void blur(Shader& blurShader, int activeCascades = -1);
    
    /**
     * Apply blur to a single cascade for selective performance optimization
     * 
     * @param blurShader     Shader for blur computation
     * @param cascadeIndex   Index of cascade to blur
     */
    void blurSingleCascade(Shader& blurShader, int cascadeIndex);
    
    // Screen Space Ambient Occlusion (SSAO)
    
    /**
     * Compute Screen Space Ambient Occlusion
     * Provides contact shadows and enhanced depth perception
     * 
     * @param ssaoShader SSAO computation shader
     * @param projection Camera projection matrix for depth reconstruction
     */
    void computeSSAO(Shader& ssaoShader, const glm::mat4& projection);
    
    /**
     * Apply blur to SSAO to reduce noise while preserving details
     * 
     * @param blurShader SSAO blur shader
     */
    void blurSSAO(Shader& blurShader);
    
    // Screen Space Reflections (SSR)
    
    /**
     * Compute Screen Space Reflections
     * Provides realistic surface reflections for shiny materials
     * 
     * @param ssrShader SSR computation shader
     * @param colorTexture Current frame color texture for reflection sampling
     * @param view Current view matrix
     * @param projection Current projection matrix
     * @param viewPos Camera position in world space
     */
    void computeSSR(Shader& ssrShader, unsigned int colorTexture, const glm::mat4& view, 
                    const glm::mat4& projection, const glm::vec3& viewPos);
    
    /**
     * Apply SSR to final composite
     * Blends screen space reflections with the lit scene
     * 
     * @param compositeShader Final composite shader
     * @param ssrStrength Reflection strength multiplier
     */
    void applySSR(Shader& compositeShader, float ssrStrength);
    
    // Temporal Anti-Aliasing (TAA)
    
    /**
     * Apply Temporal Anti-Aliasing to reduce aliasing artifacts
     * Uses motion vectors and history buffer for temporal upsampling
     * 
     * @param taaShader TAA computation shader
     * @param currentFrame Current frame texture
     * @param currentViewProj Current view-projection matrix
     * @param previousViewProj Previous frame's view-projection matrix
     */
    void applyTAA(Shader& taaShader, unsigned int currentFrame, 
                  const glm::mat4& currentViewProj, const glm::mat4& previousViewProj);
    
    /**
     * Apply FXAA (Fast Approximate Anti-Aliasing) as alternative to TAA
     * 
     * @param fxaaShader FXAA computation shader
     * @param inputTexture Source texture to anti-alias
     */
    void applyFXAA(Shader& fxaaShader, unsigned int inputTexture);
    
    // System Management
    
    /**
     * Resize all buffers when window size changes
     * Reallocates all framebuffers and textures to new resolution
     * 
     * @param width  New screen width
     * @param height New screen height
     */
    void resize(int width, int height);
    
    /**
     * Reset temporal accumulation buffers
     * Useful when lighting changes dramatically to prevent ghosting
     */
    void resetTemporalAccumulation();
    
    /**
     * Enable or disable temporal accumulation
     * Useful for eliminating ghosting artifacts
     */
    void setTemporalAccumulation(bool enabled);
    
    // Texture Access Methods
    
    /**
     * Get radiance cascade texture for a specific level
     * 
     * @param cascade Cascade level (0 = finest, higher = coarser)
     * @return OpenGL texture ID for the cascade
     */
    unsigned int getTexture(int cascade) const;
    
    /**
     * Get framebuffer for a specific cascade level
     * 
     * @param cascade Cascade level
     * @return OpenGL framebuffer ID
     */
    unsigned int getCascadeFBO(int cascade) const;
    
    // G-Buffer Access Methods
    
    /**
     * Get world-space position buffer from G-buffer
     * Contains XYZ world positions for each pixel
     */
    unsigned int getGPosition() const;
    
    /**
     * Get world-space normal buffer from G-buffer  
     * Contains normalized surface normals for lighting calculations
     */
    unsigned int getGNormal() const;
    
    /**
     * Get albedo (base color) buffer from G-buffer
     * Contains material diffuse colors
     */
    unsigned int getGAlbedo() const;
    
    /**
     * Get motion vector buffer for temporal effects
     * Contains screen-space velocity for TAA and temporal filtering
     */
    unsigned int getGVelocity() const;
    
    /**
     * Get emission buffer from G-buffer
     * Contains emissive color and intensity for light-emitting surfaces
     */
    unsigned int getGEmission() const;
    
    /**
     * Get history texture for Temporal Anti-Aliasing
     * Contains previous frame's result for temporal upsampling
     */
    unsigned int getHistoryTexture() const;
    
    // SSAO Access Methods
    
    /**
     * Get raw SSAO texture (before blur)
     * Contains ambient occlusion values
     */
    unsigned int getSSAOTexture() const { return ssaoTexture; }
    
    /**
     * Get blurred SSAO texture (final result)
     * Contains noise-reduced ambient occlusion for final rendering
     */
    unsigned int getSSAOBlurTexture() const { return ssaoBlurTexture; }
    
    /**
     * Get SSR reflection texture
     * Contains computed screen space reflections
     */
    unsigned int getSSRTexture() const { return ssrTexture; }
    
    /**
     * Get TAA output texture
     * Contains temporally anti-aliased frame
     */
    unsigned int getTAATexture() const { return taaTexture; }
    
    // Framebuffer Binding Methods
    
    /**
     * Bind G-buffer for writing geometry data
     * Sets up MRT (Multiple Render Targets) for deferred rendering
     */
    void bindGBufferForWriting();
    
    /**
     * Bind cascade textures for reading during GI computation
     */
    void bindForReading();
    
    /**
     * Initialize G-buffer with proper formats and attachments
     */
    void setupGBuffer();
    
    // Utility Methods
    
    /**
     * Get width of a specific cascade level
     * Cascades have progressively lower resolution
     * 
     * @param cascade Cascade level
     * @return Width in pixels
     */
    int getCascadeWidth(int cascade) const;
    
    /**
     * Get height of a specific cascade level
     * 
     * @param cascade Cascade level  
     * @return Height in pixels
     */
    int getCascadeHeight(int cascade) const;

private:
    // Core Properties
    int screenWidth, screenHeight;              ///< Current screen resolution
    int numCascades;                           ///< Number of cascade levels
    
    // Cascade Resources
    std::vector<unsigned int> cascadeFBOs;     ///< Framebuffers for each cascade level
    std::vector<unsigned int> cascadeTextures; ///< Radiance textures for each cascade
    std::vector<unsigned int> tempBlurFBOs;    ///< Temporary framebuffers for blur passes
    std::vector<unsigned int> tempBlurTextures;///< Temporary textures for blur passes
    
    // Temporal Accumulation (for stability and convergence)
    std::vector<unsigned int> temporalFBOs;    ///< Temporal accumulation framebuffers
    std::vector<unsigned int> temporalTextures;///< Temporal accumulation textures
    bool useTemporalBuffer;                    ///< Flag for temporal buffer usage
    int frameCounter;                          ///< Frame counter for temporal effects
    
    // G-Buffer Resources (Deferred Rendering)
    unsigned int gBuffer;                      ///< Main G-buffer framebuffer
    unsigned int gPosition;                    ///< World-space position texture (RGB: xyz, A: depth)
    unsigned int gNormal;                      ///< World-space normal texture (RGB: normalized xyz)
    unsigned int gAlbedo;                      ///< Albedo/diffuse color texture (RGB: color, A: roughness)
    unsigned int gDepth;                       ///< Depth buffer for depth testing
    unsigned int rboDepth;                     ///< Depth renderbuffer object
    unsigned int gVelocity;                    ///< Motion vector texture for TAA (RG: screen-space velocity)
    unsigned int gEmission;                    ///< Emission texture (RGB: emissive color and intensity)
    unsigned int historyTexture;               ///< Previous frame texture for TAA
    
    // Cascade Parameters
    float probeSpacing;                        ///< Spatial spacing between radiance probes
    float angularResolution;                   ///< Angular resolution for directional sampling
    std::vector<int> cascadeWidths;           ///< Width of each cascade level
    std::vector<int> cascadeHeights;          ///< Height of each cascade level
    
    // SSAO Resources
    unsigned int ssaoFBO, ssaoBlurFBO;        ///< SSAO framebuffers (raw and blurred)
    unsigned int ssaoTexture, ssaoBlurTexture;///< SSAO textures (raw and blurred)
    unsigned int noiseTexture;                ///< Random noise texture for SSAO sampling
    std::vector<glm::vec3> ssaoKernel;        ///< Hemisphere sampling kernel for SSAO
    
    // SSR Resources
    unsigned int ssrFBO;                      ///< SSR framebuffer for reflection computation
    unsigned int ssrTexture;                  ///< SSR reflection texture (RGB: reflection, A: strength)
    
    // TAA Resources
    unsigned int taaFBO;                      ///< TAA framebuffer for temporal accumulation
    unsigned int taaTexture;                  ///< TAA output texture
    
    // Private Setup Methods
    
    /**
     * Initialize all cascade framebuffers and textures
     */
    void setupCascades();
    
    /**
     * Initialize blur target resources
     */
    void setupBlurTargets();
    
    /**
     * Initialize temporal accumulation buffers
     */
    void setupTemporalBuffers();
    
    /**
     * Initialize Temporal Anti-Aliasing resources
     */
    void setupTAA();
    
    /**
     * Initialize Screen Space Ambient Occlusion resources
     */
    void setupSSAO();
    
    /**
     * Generate hemisphere sampling kernel for SSAO
     * Creates a randomized set of sample points for ambient occlusion
     */
    void generateSSAOKernel();
    
    /**
     * Generate noise texture for SSAO sampling
     * Provides random rotation vectors to reduce banding artifacts
     */
    void generateNoiseTexture();
    
    /**
     * Initialize Screen Space Reflection resources
     */
    void setupSSR();
    
    /**
     * Clean up all OpenGL resources
     */
    void cleanup();
};

#endif 