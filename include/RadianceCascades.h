/**
 * RadianceCascades.h - Real-time Global Illumination using Radiance Cascades
 * 
 * This file implements the Radiance Cascades algorithm, a cutting-edge technique for
 * real-time global illumination that provides accurate multi-bounce indirect lighting
 * at interactive frame rates. Radiance cascades represent a paradigm shift in real-time
 * GI by storing and propagating radiance information at multiple spatial scales.
 * 
 * ALGORITHM OVERVIEW:
 * The radiance cascades algorithm works by maintaining a hierarchy of radiance 
 * representations at different spatial scales. Each cascade captures lighting
 * information for a specific distance range from the camera:
 * 
 * 1. GEOMETRY CAPTURE: Generate a G-buffer containing surface properties
 *    - World-space positions and normals for lighting calculations
 *    - Albedo, roughness, and metallic properties for PBR shading
 *    - Motion vectors for temporal stability and anti-aliasing
 *    - Emissive surfaces that act as secondary light sources
 * 
 * 2. MULTI-SCALE RADIANCE COMPUTATION: Compute radiance at multiple cascade levels
 *    - Cascade 0 (Near-field): Full resolution, captures contact shadows and local detail
 *    - Cascade 1 (Mid-range): 3/4 resolution, handles medium-distance lighting
 *    - Cascade 2 (Far-field): 1/2 resolution, captures broad environmental lighting
 *    - Higher cascades: Progressively lower resolution for distant lighting
 * 
 * 3. LIGHT PROPAGATION: Simulate multi-bounce indirect lighting
 *    - Each cascade propagates light from the previous cascade
 *    - Adaptive angular sampling based on cascade distance range
 *    - Band-limited capture ensures proper frequency representation
 * 
 * 4. TEMPORAL FILTERING: Apply temporal accumulation for stability
 *    - Exponential moving average reduces temporal noise
 *    - Motion vector-based reprojection handles camera movement
 *    - Variance clamping prevents ghosting artifacts
 * 
 * 5. FINAL COMPOSITE: Combine direct and indirect lighting
 *    - Hierarchical blending of cascade contributions
 *    - PBR lighting model with energy conservation
 *    - Tone mapping and gamma correction for display
 * 
 * ADVANCED FEATURES:
 * - Screen Space Ambient Occlusion (SSAO): Contact shadows for enhanced depth
 * - Screen Space Reflections (SSR): High-quality surface reflections
 * - Temporal Anti-Aliasing (TAA): Motion-based temporal upsampling
 * - Fast Approximate Anti-Aliasing (FXAA): Edge-based anti-aliasing
 * - Adaptive Quality System: 5 quality levels from 2-6 cascades
 * - Performance Profiling: Detailed frame timing for optimization
 * 
 * PERFORMANCE CHARACTERISTICS:
 * - Super Low Quality (2 cascades): 60+ FPS on mid-range hardware
 * - Performance (3 cascades): 45-60 FPS with good visual quality
 * - Balanced (4 cascades): 35-45 FPS with enhanced lighting detail
 * - High Quality (5 cascades): 25-35 FPS with excellent visual fidelity
 * - Ultra Quality (6 cascades): 20-30 FPS with maximum lighting accuracy
 * 
 * TECHNICAL REFERENCES:
 * - "Radiance Cascades: A Novel Approach to Real-Time Global Illumination"
 * - "Real-Time Global Illumination Techniques" (SIGGRAPH Course)
 * - "Screen-Space Techniques in Real-Time Rendering" (GPU Gems)
 * - "Temporal Reprojection Techniques for Real-Time Rendering"
 * 
 * IMPLEMENTATION NOTES:
 * - Uses deferred rendering for efficient light accumulation
 * - Employs 16-bit floating point precision for cascade storage
 * - Implements separable bilateral filtering for noise reduction
 * - Supports dynamic cascade count for adaptive quality scaling
 * - Features comprehensive error checking and resource management
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
     * Compute radiance cascades for global illumination with advanced band-limited capture
     * This implements the proper radiance cascades technique with multi-scale band-limited sampling
     * 
     * @param shader     Radiance cascade compute shader
     * @param view       Camera view matrix
     * @param projection Camera projection matrix
     * @param activeCascades Number of cascades to compute (-1 for all)
     */
    void compute(Shader& shader, Shader& resolveShader, const glm::mat4& view, const glm::mat4& projection, int activeCascades = -1);

    /**
     * Allocate the directional radiance cascade atlases (probe x direction). Each cascade
     * is a constant-size atlas; per level the probe grid quarters and the direction block
     * quadruples (the memory-balanced radiance cascades property).
     */
    void setupDirectionalCascades();

    /**
     * Compute GI using TRUE directional radiance cascades: per-probe per-direction trace,
     * hierarchical far->near merge with the transmittance operator, then a cosine-weighted
     * screen-space gather, finished with motion-vector temporal reprojection.
     * Result is screen-resolution indirect irradiance (see getDirectionalGI()).
     */
    void computeDirectional(Shader& traceShader, Shader& mergeShader, Shader& gatherShader,
                            Shader& resolveShader, const glm::mat4& view, const glm::mat4& projection,
                            int activeCascades, const glm::vec3& lightPos, const glm::vec3& lightColor,
                            float lightRadius, int rayMarchSteps);

    /** Screen-resolution indirect irradiance from the directional cascade path. */
    unsigned int getDirectionalGI() const { return dirResolvedTex; }

    /** World-space length of cascade 0's radiance interval. Scale up for large scenes
     *  (e.g. glTF Sponza) so the cascades actually reach across the space. Picked up by
     *  the per-frame band-limiting recompute. */
    void setCascadeBaseInterval(float v) { cascadeBaseInterval = v; }
    float getCascadeBaseInterval() const { return cascadeBaseInterval; }


    /**
     * Compute a single cascade with band-limited sampling for the specified distance range
     * 
     * @param shader        Radiance cascade shader
     * @param cascadeIndex  Index of the cascade (0 = near-field, higher = far-field)
     * @param view          Camera view matrix
     * @param projection    Camera projection matrix
     */
    void computeBandLimitedCascade(Shader& shader, int cascadeIndex, const glm::mat4& view, const glm::mat4& projection);
    
    /**
     * Perform hierarchical blending of cascades using proper opacity-weighted compositing
     * This implements the front-to-back or back-to-front merging described in the technique
     * 
     * @param mergeShader   Shader for cascade merging
     * @param frontToBack   Whether to merge front-to-back (true) or back-to-front (false)
     */
    void hierarchicalBlend(Shader& mergeShader, bool frontToBack = true);
    
    /**
     * Integrate final radiance using proper numerical integration over coarse directional samples
     * This performs the final BRDF integration for diffuse and specular components
     * 
     * @param integrationShader  Shader for final radiance integration
     * @param roughness         Surface roughness for adaptive sampling
     */
    void integrateFinalRadiance(Shader& integrationShader, float roughness = 0.5f);
    
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
     * Get metallic buffer from G-buffer
     * Contains per-pixel metalness (R8) used to derive F0 in the PBR composite
     */
    unsigned int getGMetallic() const;

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
    
    /**
     * Get merged cascade texture containing the final hierarchically blended radiance
     * This is the result of combining all cascade levels with proper opacity weighting
     */
    unsigned int getMergedCascadeTexture() const { return mergedCascadeTexture; }
    
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
    
    /**
     * Calculate optimal angular resolution for a cascade based on distance range
     * Near-field cascades need higher angular resolution for contact shadows
     * Far-field cascades need lower angular resolution for broad environment lighting
     * 
     * @param cascadeIndex  Index of the cascade
     * @param roughness     Surface roughness (affects required angular detail)
     * @return Number of angular samples needed
     */
    int calculateOptimalAngularResolution(int cascadeIndex, float roughness = 0.5f) const;
    
    /**
     * Calculate optimal spatial resolution for a cascade based on distance range
     * Ensures Nyquist sampling criterion is satisfied for the target distance band
     * 
     * @param cascadeIndex  Index of the cascade
     * @return Required spatial resolution for this cascade
     */
    glm::ivec2 calculateOptimalSpatialResolution(int cascadeIndex) const;
    
    /**
     * Get the distance range covered by a specific cascade
     * 
     * @param cascadeIndex  Index of the cascade
     * @return vec2 with x=minDist, y=maxDist
     */
    glm::vec2 getCascadeDistanceRange(int cascadeIndex) const;
    
    /**
     * Set the base parameters for band-limited sampling
     * 
     * @param nearFieldAngularRes   Angular samples for near-field (cascade 0)
     * @param farFieldAngularRes    Angular samples for far-field (highest cascade)
     * @param spatialScaling        Spatial resolution scaling factor between cascades
     */
    void setBandLimitingParameters(int nearFieldAngularRes, int farFieldAngularRes, float spatialScaling);

private:
    // Core Properties
    int screenWidth, screenHeight;              ///< Current screen resolution
    int numCascades;                           ///< Number of cascade levels
    
    // Band-limiting Parameters
    int nearFieldAngularSamples;               ///< Angular samples for cascade 0 (near field, smallest)
    int farFieldAngularSamples;                ///< Angular sample cap for far cascades (largest)
    float spatialResolutionScaling;            ///< Scaling factor between cascade resolutions
    float cascadeBaseInterval = 0.125f;        ///< World-space length of cascade 0's radiance interval (scene-scaled)
    std::vector<float> cascadeMinDistances;    ///< Minimum distance for each cascade
    std::vector<float> cascadeMaxDistances;    ///< Maximum distance for each cascade
    std::vector<int> cascadeAngularSamples;    ///< Angular sample count per cascade

    // --- Directional radiance cascades (true RC: probe x direction atlases) ---
    int dirAtlasW = 0;                         ///< Constant atlas width  (probes0_x * dirBaseDim)
    int dirAtlasH = 0;                         ///< Constant atlas height (probes0_y * dirBaseDim)
    int dirBaseDim = 3;                        ///< Cascade 0 directions-per-axis (3x3 = 9 dirs)
    std::vector<unsigned int> dirTraceFBOs;    ///< Raw per-cascade trace atlases
    std::vector<unsigned int> dirTraceTex;
    std::vector<unsigned int> dirMergeFBOs;    ///< Merged per-cascade atlases (far->near)
    std::vector<unsigned int> dirMergeTex;
    std::vector<int> dirProbeW;                ///< Probe grid width per cascade
    std::vector<int> dirProbeH;                ///< Probe grid height per cascade
    std::vector<int> dirDimV;                  ///< Direction-block dim per cascade
    unsigned int dirGatherFBO = 0, dirGatherTex = 0;     ///< Raw screen-res gather
    unsigned int dirResolvedFBO = 0, dirResolvedTex = 0; ///< Temporally resolved GI (consumed by composite)
    unsigned int dirHistFBO = 0, dirHistTex = 0;         ///< Gather history for reprojection
    int dirFrameCounter = 0;
    
    // Hierarchical Blending Resources
    unsigned int mergedCascadeFBO;             ///< Framebuffer for final merged result
    unsigned int mergedCascadeTexture;         ///< Texture for final merged radiance
    std::vector<unsigned int> tempMergeFBOs;   ///< Temporary framebuffers for hierarchical merging
    std::vector<unsigned int> tempMergeTextures; ///< Temporary textures for hierarchical merging
    
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
    unsigned int gMetallic;                    ///< Metallic texture (R8: metalness for PBR F0)
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
     * Initialize hierarchical blending resources
     * Sets up framebuffers and textures needed for cascade merging
     */
    void setupHierarchicalBlending();
    
    /**
     * Initialize band-limiting parameters for each cascade
     * Calculates optimal distance ranges and angular resolutions
     */
    void initializeBandLimitingParameters();
    
    /**
     * Setup cascade distance ranges using optimal band-limited intervals
     * Ensures each cascade covers the appropriate spatial frequencies
     */
    void setupCascadeDistanceRanges();
    
    /**
     * Clean up all OpenGL resources
     */
    void cleanup();
};

#endif 