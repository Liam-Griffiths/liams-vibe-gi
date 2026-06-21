#include "../include/RadianceCascades.h"
#include "../include/GLHeaders.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <random>
#include <cstdlib>
#include "../include/FullscreenQuad.h"

// Remove global variables that conflict with member variables

RadianceCascades::RadianceCascades(int width, int height, int num, float baseSpacing, float angularBase) : screenWidth(width), screenHeight(height), numCascades(num), probeSpacing(baseSpacing), angularResolution(angularBase), rboDepth(0), useTemporalBuffer(true), frameCounter(0), historyTexture(0), nearFieldAngularSamples(48), farFieldAngularSamples(12), spatialResolutionScaling(0.6f) {
    // Initialize band-limiting parameters
    initializeBandLimitingParameters();
    
    setupGBuffer();
    setupCascades();
    setupBlurTargets();
    setupTemporalBuffers();
    // setupHierarchicalBlending(); // Disabled for performance - causing expensive GPU operations
    setupTAA(); // Add TAA setup
    setupSSAO(); // Add SSAO setup
    setupSSR(); // Add SSR setup
}

RadianceCascades::~RadianceCascades() {
    cleanup();
}

// New TAA setup
void RadianceCascades::setupTAA() {
    glGenTextures(1, &historyTexture);
    glBindTexture(GL_TEXTURE_2D, historyTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Clear using FBO for compatibility
    GLuint tempFBO;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTexture, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &tempFBO);
}

void RadianceCascades::setupCascades() {
    cascadeFBOs.resize(numCascades);
    cascadeTextures.resize(numCascades);
    cascadeWidths.resize(numCascades);
    cascadeHeights.resize(numCascades);
    glGenFramebuffers(numCascades, cascadeFBOs.data());
    glGenTextures(numCascades, cascadeTextures.data());

    for (int i = 0; i < numCascades; ++i) {
        // ENHANCED CASCADE RESOLUTIONS for ultra-smooth GI
        // GI is traced at HALF the previous resolution at every cascade level: the
        // dominant cost is the per-pixel hemisphere march, and the composite already
        // applies a depth/normal-aware bilateral upsample (bilateralGI), so the lower
        // trace resolution is reconstructed cleanly without visible bleeding.
        int res_x, res_y;
        if (i == 0) {
            // Cascade 0: half resolution (was full) - ~4x fewer traced pixels
            res_x = screenWidth / 2;
            res_y = screenHeight / 2;
        } else if (i == 1) {
            // Cascade 1: was 3/4 -> 3/8 of screen
            res_x = (screenWidth * 3) / 8;
            res_y = (screenHeight * 3) / 8;
        } else if (i == 2) {
            // Cascade 2: was 1/2 -> 1/4 of screen
            res_x = screenWidth / 4;
            res_y = screenHeight / 4;
        } else {
            // Higher cascades: one extra halving on top of the previous scheme
            res_x = std::max(96, screenWidth >> (i + 1));
            res_y = std::max(96, screenHeight >> (i + 1));
        }
        cascadeWidths[i] = res_x;
        cascadeHeights[i] = res_y;

        glBindTexture(GL_TEXTURE_2D, cascadeTextures[i]);
        // Use 16-bit precision for all cascades for bandwidth; avoids format conversion on blits
        GLenum internalFormat = GL_RGBA16F;
        GLenum dataType = GL_HALF_FLOAT;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, res_x, res_y, 0, GL_RGBA, dataType, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascadeTextures[i], 0);
        
        // Check framebuffer completeness for each cascade
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Cascade framebuffer " << i << " incomplete!" << std::endl;
        }
    }
}

void RadianceCascades::setupGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // Position buffer (16-bit is sufficient for most scenes)
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // Normal buffer (RG16F - reconstruct Z component for bandwidth savings)
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, screenWidth, screenHeight, 0, GL_RG, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // Albedo (8-bit is perfect for colors)
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

    // Linear Depth (16-bit sufficient for depth)
    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, screenWidth, screenHeight, 0, GL_RED, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gDepth, 0);

    // Velocity buffer (16-bit RGB for motion vectors)
    glGenTextures(1, &gVelocity);
    glBindTexture(GL_TEXTURE_2D, gVelocity);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, screenWidth, screenHeight, 0, GL_RG, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gVelocity, 0);

    // Emission buffer (16-bit RGB for HDR emission values)
    glGenTextures(1, &gEmission);
    glBindTexture(GL_TEXTURE_2D, gEmission);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, gEmission, 0);

    // Depth renderbuffer (24-bit is standard and efficient)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[6] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5};
    glDrawBuffers(6, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "G-buffer incomplete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::setupBlurTargets() {
    tempBlurFBOs.resize(numCascades);
    tempBlurTextures.resize(numCascades);
    glGenFramebuffers(numCascades, tempBlurFBOs.data());
    glGenTextures(numCascades, tempBlurTextures.data());

    for (int i = 0; i < numCascades; ++i) {
        int res_x = cascadeWidths[i];
        int res_y = cascadeHeights[i];

        glBindTexture(GL_TEXTURE_2D, tempBlurTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, res_x, res_y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, tempBlurFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempBlurTextures[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Blur framebuffer " << i << " incomplete!" << std::endl;
        }
    }
}

void RadianceCascades::setupTemporalBuffers() {
    temporalFBOs.resize(numCascades);
    temporalTextures.resize(numCascades);
    glGenFramebuffers(numCascades, temporalFBOs.data());
    glGenTextures(numCascades, temporalTextures.data());

    for (int i = 0; i < numCascades; ++i) {
        int res_x = cascadeWidths[i];
        int res_y = cascadeHeights[i];

        glBindTexture(GL_TEXTURE_2D, temporalTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, res_x, res_y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, temporalFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temporalTextures[i], 0);
        
        // Clear temporal buffers to black initially
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Temporal framebuffer " << i << " incomplete!" << std::endl;
        }
    }
}

void RadianceCascades::blur(Shader& blurShader, int activeCascades) {
    if (activeCascades == -1) activeCascades = numCascades; // Use all cascades by default
    blurShader.use();
    static FullscreenQuad quad; // Reuse across calls to avoid VAO/VBO churn
    
    // Apply blur to active cascades for consistent smoothing
    for (int i = 0; i < activeCascades; ++i) {
        int res_x = cascadeWidths[i];
        int res_y = cascadeHeights[i];
        
        // Apply blur to ALL cascades now that we have better resolutions
        // Even small cascades benefit from denoising
        
        // PASS 1: Horizontal blur (cascade -> temp)
        glBindFramebuffer(GL_FRAMEBUFFER, tempBlurFBOs[i]);
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT);
        
        blurShader.setInt("blurDirection", 0); // Horizontal
        blurShader.setInt("inputTexture", 0);
        blurShader.setInt("gPosition", 1);
        blurShader.setInt("gNormal", 2);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cascadeTextures[i]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        
        quad.render();
        
        // PASS 2: Vertical blur (temp -> cascade)
        glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT);
        
        blurShader.setInt("blurDirection", 1); // Vertical
        blurShader.setInt("inputTexture", 0);
        blurShader.setInt("gPosition", 1);
        blurShader.setInt("gNormal", 2);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tempBlurTextures[i]); // Use temp texture as input
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        
        quad.render();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void RadianceCascades::bindGBufferForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RadianceCascades::bindForReading() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, gEmission); // Add emission texture for GI (unit 6 to avoid conflicts)
}

void RadianceCascades::cleanup() {
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedo);
    glDeleteTextures(1, &gDepth);
    glDeleteTextures(1, &gVelocity); // Added gVelocity cleanup
    glDeleteTextures(1, &gEmission); // Added gEmission cleanup
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteFramebuffers(numCascades, cascadeFBOs.data());
    glDeleteTextures(numCascades, cascadeTextures.data());
    glDeleteFramebuffers(numCascades, tempBlurFBOs.data());
    glDeleteTextures(numCascades, tempBlurTextures.data());
    glDeleteFramebuffers(numCascades, temporalFBOs.data());
    glDeleteTextures(numCascades, temporalTextures.data());
    glDeleteTextures(1, &historyTexture);

    // Directional radiance cascade cleanup
    if (!dirTraceFBOs.empty()) {
        glDeleteFramebuffers(numCascades, dirTraceFBOs.data());
        glDeleteTextures(numCascades, dirTraceTex.data());
        glDeleteFramebuffers(numCascades, dirMergeFBOs.data());
        glDeleteTextures(numCascades, dirMergeTex.data());
    }
    glDeleteFramebuffers(1, &dirGatherFBO);
    glDeleteTextures(1, &dirGatherTex);
    glDeleteFramebuffers(1, &dirResolvedFBO);
    glDeleteTextures(1, &dirResolvedTex);
    glDeleteFramebuffers(1, &dirHistFBO);
    glDeleteTextures(1, &dirHistTex);

    // Hierarchical blending cleanup - disabled for performance
    /*
    glDeleteFramebuffers(1, &mergedCascadeFBO);
    glDeleteTextures(1, &mergedCascadeTexture);
    if (!tempMergeFBOs.empty()) {
        glDeleteFramebuffers(numCascades, tempMergeFBOs.data());
        glDeleteTextures(numCascades, tempMergeTextures.data());
    }
    */
    
    // SSAO cleanup
    glDeleteFramebuffers(1, &ssaoFBO);
    glDeleteFramebuffers(1, &ssaoBlurFBO);
    glDeleteTextures(1, &ssaoTexture);
    glDeleteTextures(1, &ssaoBlurTexture);
    glDeleteTextures(1, &noiseTexture);
    
    // SSR cleanup
    glDeleteFramebuffers(1, &ssrFBO);
    glDeleteTextures(1, &ssrTexture);
    
    // TAA cleanup
    glDeleteFramebuffers(1, &taaFBO);
    glDeleteTextures(1, &taaTexture);
}

void RadianceCascades::resize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    frameCounter = 0; // Reset temporal accumulation on resize
    cleanup();
    setupGBuffer();
    setupCascades();
    setupBlurTargets();
    setupTemporalBuffers();
    setupTAA(); // Re-setup TAA on resize
    setupSSAO(); // Re-setup SSAO on resize
    setupSSR(); // Re-setup SSR on resize
}

void RadianceCascades::compute(Shader& shader, Shader& resolveShader, const glm::mat4& view, const glm::mat4& projection, int activeCascades) {
    if (activeCascades == -1) activeCascades = numCascades; // Use all cascades by default
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("invView", glm::inverse(view)); // Add inverse view for world space calculations
    shader.setFloat("time", glfwGetTime());
    shader.setInt("frameCounter", frameCounter);
    // Temporal accumulation is handled by the separate resolve pass below, so the trace
    // pass always renders a raw (non-temporal) estimate.
    shader.setBool("useTemporalAccumulation", false);
    shader.setInt("gPosition", 0);
    shader.setInt("gNormal", 1);
    shader.setInt("gAlbedo", 2);
    shader.setInt("gLinearDepth", 3);
    shader.setInt("gEmission", 6); // Add emission texture for GI calculations (avoid conflict with previousCascade)

    bindForReading();
    static FullscreenQuad quad; // Reuse

    bool useTemporal = useTemporalBuffer && frameCounter > 0;

    for (int i = activeCascades - 1; i >= 0; --i) {
        int res_x = cascadeWidths[i];
        int res_y = cascadeHeights[i];

        // ---- PASS A: trace raw GI into the scratch (tempBlur) target ----
        shader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, tempBlurFBOs[i]);
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.setInt("cascadeIndex", i);
        // Band-limited cascade parameters from precomputed ranges
        glm::vec2 distRange = getCascadeDistanceRange(i);
        float minDistance = distRange.x;
        float maxDistance = distRange.y;
        shader.setFloat("minDistance", minDistance);
        shader.setFloat("maxDistance", maxDistance);

        // Angular samples from precomputed cascade settings
        shader.setInt("angularSamples", cascadeAngularSamples[i]);

        // Controlled overlap window (~15%) for smooth inter-cascade blending
        float overlapStart = glm::mix(minDistance, maxDistance, 0.85f);
        float overlapEnd   = glm::mix(minDistance, maxDistance, 1.00f);
        shader.setFloat("cascadeOverlapStart", overlapStart);
        shader.setFloat("cascadeOverlapEnd", overlapEnd);
        shader.setBool("enableCascadeBlending", i > 0);

        // Bind previous (coarser) cascade for the spatial hierarchy. cascadeTextures[i+1]
        // already holds the temporally-resolved result from the previous loop iteration.
        if (i < numCascades - 1) {
            shader.setInt("previousCascade", 4);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, cascadeTextures[i+1]);
            shader.setBool("hasPreviousCascade", true);
        } else {
            shader.setBool("hasPreviousCascade", false);
        }

        quad.render();

        // ---- PASS B: temporal resolve (reproject + variance clamp) into the cascade ----
        glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT);

        resolveShader.use();
        resolveShader.setInt("cascadeIndex", i);
        resolveShader.setBool("hasHistory", useTemporal);
        resolveShader.setInt("currentGI", 0);
        resolveShader.setInt("historyGI", 1);
        resolveShader.setInt("gVelocity", 2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tempBlurTextures[i]); // raw current frame
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, temporalTextures[i]); // history
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gVelocity);

        quad.render();

        // Restore the G-buffer bindings on units 0-3/6 for the next cascade's trace pass.
        bindForReading();

        // ---- Update history for next frame ----
        if (useTemporalBuffer) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, cascadeFBOs[i]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temporalFBOs[i]);
            glBlitFramebuffer(0, 0, res_x, res_y, 0, 0, res_x, res_y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }

    frameCounter++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

// ============================================================================
// Directional radiance cascades (true RC: probe x direction atlases)
// ============================================================================

void RadianceCascades::setupDirectionalCascades() {
    int p0w = std::max(1, screenWidth / 2);  // cascade 0 probe grid = half screen res
    int p0h = std::max(1, screenHeight / 2);
    dirAtlasW = p0w * dirBaseDim;             // constant atlas size for every cascade
    dirAtlasH = p0h * dirBaseDim;

    dirTraceFBOs.resize(numCascades); dirTraceTex.resize(numCascades);
    dirMergeFBOs.resize(numCascades); dirMergeTex.resize(numCascades);
    dirProbeW.resize(numCascades); dirProbeH.resize(numCascades); dirDimV.resize(numCascades);

    glGenFramebuffers(numCascades, dirTraceFBOs.data());
    glGenTextures(numCascades, dirTraceTex.data());
    glGenFramebuffers(numCascades, dirMergeFBOs.data());
    glGenTextures(numCascades, dirMergeTex.data());

    for (int i = 0; i < numCascades; ++i) {
        int dim = dirBaseDim << i;            // directions-per-axis doubles per cascade
        dirDimV[i] = dim;
        dirProbeW[i] = std::max(1, dirAtlasW / dim);  // probes halve per axis per cascade
        dirProbeH[i] = std::max(1, dirAtlasH / dim);

        for (int pass = 0; pass < 2; ++pass) {
            unsigned int tex = (pass == 0) ? dirTraceTex[i] : dirMergeTex[i];
            unsigned int fbo = (pass == 0) ? dirTraceFBOs[i] : dirMergeFBOs[i];
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, dirAtlasW, dirAtlasH, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
            // NEAREST: the atlas tiles direction cells, so filtering would bleed across them.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Directional cascade FBO " << i << " incomplete!" << std::endl;
        }
    }

    // Screen-resolution gather / temporally-resolved / history buffers.
    auto makeScreenTex = [&](unsigned int& fbo, unsigned int& tex) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    };
    makeScreenTex(dirGatherFBO, dirGatherTex);
    makeScreenTex(dirResolvedFBO, dirResolvedTex);
    makeScreenTex(dirHistFBO, dirHistTex);

    dirFrameCounter = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::computeDirectional(Shader& traceShader, Shader& mergeShader, Shader& gatherShader,
        Shader& resolveShader, const glm::mat4& view, const glm::mat4& projection,
        int activeCascades, const glm::vec3& lightPos, const glm::vec3& lightColor,
        float lightRadius, int rayMarchSteps) {
    if (activeCascades <= 0 || activeCascades > numCascades) activeCascades = numCascades;
    static FullscreenQuad quad;
    glm::mat4 invView = glm::inverse(view);

    // ---- PASS A: trace every active cascade (one global direction per atlas texel) ----
    traceShader.use();
    traceShader.setMat4("view", view);
    traceShader.setMat4("projection", projection);
    traceShader.setMat4("invView", invView);
    traceShader.setVec3("lightPos", lightPos);
    traceShader.setVec3("lightColor", lightColor);
    traceShader.setFloat("lightRadius", lightRadius);
    traceShader.setInt("rayMarchSteps", rayMarchSteps);
    traceShader.setInt("gPosition", 0);
    traceShader.setInt("gNormal", 1);
    traceShader.setInt("gAlbedo", 2);
    traceShader.setInt("gLinearDepth", 3);
    traceShader.setInt("gEmission", 5);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, gEmission);
    for (int i = 0; i < activeCascades; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, dirTraceFBOs[i]);
        glViewport(0, 0, dirAtlasW, dirAtlasH);
        glClear(GL_COLOR_BUFFER_BIT);
        glm::vec2 range = getCascadeDistanceRange(i);
        traceShader.setFloat("minDistance", range.x);
        traceShader.setFloat("maxDistance", range.y);
        traceShader.setInt("probeCountX", dirProbeW[i]);
        traceShader.setInt("probeCountY", dirProbeH[i]);
        traceShader.setInt("dirDim", dirDimV[i]);
        quad.render();
    }

    // ---- PASS B: hierarchical merge far -> near into dirMergeTex ----
    mergeShader.use();
    mergeShader.setInt("traceTex", 0);
    mergeShader.setInt("upperTex", 1);
    for (int i = activeCascades - 1; i >= 0; --i) {
        glBindFramebuffer(GL_FRAMEBUFFER, dirMergeFBOs[i]);
        glViewport(0, 0, dirAtlasW, dirAtlasH);
        glClear(GL_COLOR_BUFFER_BIT);
        mergeShader.setInt("dirDim", dirDimV[i]);
        mergeShader.setInt("probeCountX", dirProbeW[i]);
        mergeShader.setInt("probeCountY", dirProbeH[i]);
        bool hasUpper = (i < activeCascades - 1);
        mergeShader.setBool("hasUpper", hasUpper);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dirTraceTex[i]);
        if (hasUpper) {
            mergeShader.setInt("upperProbeCountX", dirProbeW[i + 1]);
            mergeShader.setInt("upperProbeCountY", dirProbeH[i + 1]);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, dirMergeTex[i + 1]);
        } else {
            mergeShader.setInt("upperProbeCountX", dirProbeW[i]);
            mergeShader.setInt("upperProbeCountY", dirProbeH[i]);
        }
        quad.render();
    }

    // ---- PASS C: cosine-weighted screen-space gather from cascade 0 (fully merged) ----
    gatherShader.use();
    gatherShader.setMat4("invView", invView);
    gatherShader.setInt("dirTex0", 0);
    gatherShader.setInt("gNormal", 1);
    gatherShader.setInt("gLinearDepth", 2);
    gatherShader.setInt("probeCountX", dirProbeW[0]);
    gatherShader.setInt("probeCountY", dirProbeH[0]);
    gatherShader.setInt("dirDim", dirDimV[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, dirGatherFBO);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dirMergeTex[0]);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gDepth);
    quad.render();

    // ---- PASS D: temporal reprojection of the gathered irradiance ----
    resolveShader.use();
    resolveShader.setInt("cascadeIndex", 0);
    resolveShader.setBool("hasHistory", dirFrameCounter > 0);
    resolveShader.setInt("currentGI", 0);
    resolveShader.setInt("historyGI", 1);
    resolveShader.setInt("gVelocity", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, dirResolvedFBO);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dirGatherTex);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, dirHistTex);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gVelocity);
    quad.render();

    // history <- resolved (for next frame's reprojection)
    glBindFramebuffer(GL_READ_FRAMEBUFFER, dirResolvedFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dirHistFBO);
    glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    dirFrameCounter++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void RadianceCascades::resetTemporalAccumulation() {
    frameCounter = 0;
    dirFrameCounter = 0; // also restart directional-path accumulation

    // Clear all temporal buffers
    for (int i = 0; i < numCascades; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, temporalFBOs[i]);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::setTemporalAccumulation(bool enabled) {
    useTemporalBuffer = enabled;
    if (!enabled) {
        resetTemporalAccumulation();
    }
}

unsigned int RadianceCascades::getTexture(int cascade) const {
    return cascadeTextures[cascade];
}
unsigned int RadianceCascades::getCascadeFBO(int cascade) const {
    return cascadeFBOs[cascade];
}
unsigned int RadianceCascades::getGPosition() const {
    return gPosition;
}
unsigned int RadianceCascades::getGNormal() const {
    return gNormal;
}
unsigned int RadianceCascades::getGAlbedo() const {
    return gAlbedo;
}
unsigned int RadianceCascades::getGVelocity() const { // Added getGVelocity
    return gVelocity;
}
unsigned int RadianceCascades::getGEmission() const { // Added getGEmission
    return gEmission;
}
unsigned int RadianceCascades::getHistoryTexture() const {
    return historyTexture;
}
int RadianceCascades::getCascadeWidth(int cascade) const {
    return cascadeWidths[cascade];
}
int RadianceCascades::getCascadeHeight(int cascade) const {
    return cascadeHeights[cascade];
} 

void RadianceCascades::setupSSAO() {
    generateSSAOKernel();
    generateNoiseTexture();
    
    // Create SSAO framebuffer
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    
    // SSAO texture (R8 for occlusion values)
    glGenTextures(1, &ssaoTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO framebuffer incomplete!" << std::endl;
    }
    
    // Create SSAO blur framebuffer
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    
    // SSAO blur texture
    glGenTextures(1, &ssaoBlurTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO blur framebuffer incomplete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::generateSSAOKernel() {
    ssaoKernel.clear();
    
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // Random floats between 0.0 - 1.0
    std::default_random_engine generator;
    
    for (int i = 0; i < 32; ++i) {
        // Create hemisphere samples with better distribution
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)  // Keep positive Z for hemisphere
        );
        sample = glm::normalize(sample);
        
        // Better scale distribution - more samples closer to origin
        float scale = (float)i / 32.0f;
        scale = 0.1f + scale * scale * 0.9f; // Quadratic interpolation from 0.1 to 1.0
        sample *= scale;
        
        ssaoKernel.push_back(sample);
    }
}

void RadianceCascades::generateNoiseTexture() {
    std::vector<glm::vec3> ssaoNoise;
    
    // Generate 4x4 noise texture for rotating samples
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            ((float)rand() / RAND_MAX) * 2.0f - 1.0f,
            ((float)rand() / RAND_MAX) * 2.0f - 1.0f,
            0.0f  // Keep rotation around Z axis only
        );
        ssaoNoise.push_back(noise);
    }
    
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
} 

void RadianceCascades::computeSSAO(Shader& ssaoShader, const glm::mat4& projection) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ssaoShader.use();
    ssaoShader.setMat4("projection", projection);
    
    // Send kernel samples to shader
    for (int i = 0; i < 32; ++i) {
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
    }
    
    // Bind G-buffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    ssaoShader.setInt("gPosition", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    ssaoShader.setInt("gNormal", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    ssaoShader.setInt("texNoise", 2);
    
    static FullscreenQuad quad; // Reuse
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::blurSSAO(Shader& blurShader) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    
    blurShader.use();
    
    // Bind SSAO texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    blurShader.setInt("ssaoInput", 0);
    
    // Bind G-buffer textures for bilateral filtering
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    blurShader.setInt("gPosition", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    blurShader.setInt("gNormal", 2);
    
    static FullscreenQuad quad; // Reuse
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::setupSSR() {
    // Create SSR framebuffer and texture
    glGenFramebuffers(1, &ssrFBO);
    glGenTextures(1, &ssrTexture);
    
    glBindTexture(GL_TEXTURE_2D, ssrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssrTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSR framebuffer incomplete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Create TAA framebuffer and texture
    glGenFramebuffers(1, &taaFBO);
    glGenTextures(1, &taaTexture);
    
    glBindTexture(GL_TEXTURE_2D, taaTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth, screenHeight, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, taaFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, taaTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "TAA framebuffer incomplete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::computeSSR(Shader& ssrShader, unsigned int colorTexture, const glm::mat4& view, 
                                  const glm::mat4& projection, const glm::vec3& viewPos) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ssrShader.use();
    ssrShader.setMat4("view", view);
    ssrShader.setMat4("projection", projection);
    ssrShader.setVec3("viewPos", viewPos);
    
    // Bind G-buffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    ssrShader.setInt("gPosition", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    ssrShader.setInt("gNormal", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    ssrShader.setInt("gAlbedo", 2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    ssrShader.setInt("colorTexture", 3);
    
    static FullscreenQuad quad; // Reuse
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::applyTAA(Shader& taaShader, unsigned int currentFrame, 
                                const glm::mat4& currentViewProj, const glm::mat4& previousViewProj) {
    glBindFramebuffer(GL_FRAMEBUFFER, taaFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    
    taaShader.use();
    taaShader.setMat4("currentViewProj", currentViewProj);
    taaShader.setMat4("previousViewProj", previousViewProj);
    taaShader.setFloat("frameCounter", float(frameCounter));
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentFrame);
    taaShader.setInt("currentFrame", 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, historyTexture);
    taaShader.setInt("historyFrame", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gVelocity);
    taaShader.setInt("gVelocity", 2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    taaShader.setInt("gPosition", 3);
    
    FullscreenQuad quad;
    quad.render();
    
    // Copy result to history buffer for next frame
    glBindFramebuffer(GL_READ_FRAMEBUFFER, taaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    
    // Create temporary FBO for history texture
    GLuint tempFBO;
    glGenFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempFBO);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTexture, 0);
    
    glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, 
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    
    glDeleteFramebuffers(1, &tempFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::applyFXAA(Shader& fxaaShader, unsigned int inputTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, taaFBO); // Reuse TAA FBO for FXAA output
    glClear(GL_COLOR_BUFFER_BIT);
    
    fxaaShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    fxaaShader.setInt("inputTexture", 0);
    
    FullscreenQuad quad;
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
} 

// ============================================================================
// Advanced Radiance Cascades Implementation
// ============================================================================

void RadianceCascades::initializeBandLimitingParameters() {
    cascadeMinDistances.resize(numCascades);
    cascadeMaxDistances.resize(numCascades);
    cascadeAngularSamples.resize(numCascades);
    
    // Radiance Cascades scaling laws (Sannikov; arXiv:2408.14425 S2.3).
    // Per cascade i the interval length doubles (t_i ~ 2^i) and, by the penumbra
    // criterion, angular resolution must INCREASE with distance (Dw ~ 2^i) while spatial
    // probe density decreases. The far field has high angular / low spatial frequency, so
    // far cascades carry many angular samples; the near field is the opposite.
    for (int i = 0; i < numCascades; ++i) {
        // Geometric interval progression, started small so the cascades actually tile a
        // typical (few-unit) scene instead of cascade 0 already spanning the whole room.
        cascadeMinDistances[i] = cascadeBaseInterval * pow(2.0f, float(i));
        cascadeMaxDistances[i] = cascadeBaseInterval * pow(2.0f, float(i + 1));

        // Angular samples DOUBLE per cascade (near -> far), capped at the far-field budget.
        // This is the inverse of the old (penumbra-violating) allocation and is nearly
        // free because far cascades are rendered at much lower spatial resolution.
        int samples = int(std::lround(nearFieldAngularSamples * pow(2.0f, float(i))));
        cascadeAngularSamples[i] = std::min(samples, farFieldAngularSamples);
        cascadeAngularSamples[i] = std::max(cascadeAngularSamples[i], 4);
    }
}

void RadianceCascades::setupHierarchicalBlending() {
    // Create merged cascade framebuffer and texture
    glGenFramebuffers(1, &mergedCascadeFBO);
    glGenTextures(1, &mergedCascadeTexture);
    
    glBindTexture(GL_TEXTURE_2D, mergedCascadeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, mergedCascadeFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mergedCascadeTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Merged cascade framebuffer incomplete!" << std::endl;
    }
    
    // Create temporary merge framebuffers for hierarchical blending
    tempMergeFBOs.resize(numCascades);
    tempMergeTextures.resize(numCascades);
    glGenFramebuffers(numCascades, tempMergeFBOs.data());
    glGenTextures(numCascades, tempMergeTextures.data());
    
    for (int i = 0; i < numCascades; ++i) {
        glBindTexture(GL_TEXTURE_2D, tempMergeTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, tempMergeFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempMergeTextures[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Temp merge framebuffer " << i << " incomplete!" << std::endl;
        }
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::computeBandLimitedCascade(Shader& shader, int cascadeIndex, const glm::mat4& view, const glm::mat4& projection) {
    shader.use();
    
    // Set band-limiting parameters for this cascade
    glm::vec2 distanceRange = getCascadeDistanceRange(cascadeIndex);
    int angularSamples = cascadeAngularSamples[cascadeIndex];
    
    shader.setInt("cascadeIndex", cascadeIndex);
    shader.setFloat("minDistance", distanceRange.x);
    shader.setFloat("maxDistance", distanceRange.y);
    shader.setInt("angularSamples", angularSamples);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    
    // Calculate optimal spatial resolution for this cascade
    glm::ivec2 resolution = calculateOptimalSpatialResolution(cascadeIndex);
    
    // Bind G-buffer for reading
    bindForReading();
    
    // Bind previous cascade if available (for hierarchical sampling)
    if (cascadeIndex < numCascades - 1) {
        shader.setInt("previousCascade", 4);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, cascadeTextures[cascadeIndex + 1]);
        shader.setBool("hasPreviousCascade", true);
    } else {
        shader.setBool("hasPreviousCascade", false);
    }
    
    // Render to this cascade's framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[cascadeIndex]);
    glViewport(0, 0, resolution.x, resolution.y);
    glClear(GL_COLOR_BUFFER_BIT);
    
    FullscreenQuad quad;
    quad.render();
}

void RadianceCascades::hierarchicalBlend(Shader& mergeShader, bool frontToBack) {
    mergeShader.use();
    mergeShader.setBool("frontToBack", frontToBack);
    
    FullscreenQuad quad;
    
    if (frontToBack) {
        // Front-to-back blending: Start with nearest cascade, blend in farther ones
        // Copy first cascade to merged buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cascadeFBOs[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mergedCascadeFBO);
        glBlitFramebuffer(0, 0, cascadeWidths[0], cascadeHeights[0], 
                          0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        
        // Blend each subsequent cascade
        for (int i = 1; i < numCascades; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, tempMergeFBOs[i]);
            glViewport(0, 0, screenWidth, screenHeight);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Bind accumulated result so far
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mergedCascadeTexture);
            mergeShader.setInt("accumulatedRadiance", 0);
            
            // Bind current cascade to blend
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cascadeTextures[i]);
            mergeShader.setInt("currentCascade", 1);
            
            mergeShader.setInt("cascadeIndex", i);
            mergeShader.setFloat("cascadeWeight", 1.0f / float(i + 1)); // Reduce weight for farther cascades
            
            quad.render();
            
            // Copy result back to merged buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, tempMergeFBOs[i]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mergedCascadeFBO);
            glBlitFramebuffer(0, 0, screenWidth, screenHeight, 
                              0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    } else {
        // Back-to-front blending: Start with farthest cascade, blend in nearer ones
        // Copy last cascade to merged buffer
        int lastCascade = numCascades - 1;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cascadeFBOs[lastCascade]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mergedCascadeFBO);
        glBlitFramebuffer(0, 0, cascadeWidths[lastCascade], cascadeHeights[lastCascade], 
                          0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        
        // Blend each subsequent cascade (going towards camera)
        for (int i = lastCascade - 1; i >= 0; --i) {
            glBindFramebuffer(GL_FRAMEBUFFER, tempMergeFBOs[i]);
            glViewport(0, 0, screenWidth, screenHeight);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Bind accumulated result so far
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mergedCascadeTexture);
            mergeShader.setInt("accumulatedRadiance", 0);
            
            // Bind current cascade to blend
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cascadeTextures[i]);
            mergeShader.setInt("currentCascade", 1);
            
            mergeShader.setInt("cascadeIndex", i);
            mergeShader.setFloat("cascadeWeight", float(numCascades - i) / float(numCascades)); // Higher weight for nearer cascades
            
            quad.render();
            
            // Copy result back to merged buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, tempMergeFBOs[i]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mergedCascadeFBO);
            glBlitFramebuffer(0, 0, screenWidth, screenHeight, 
                              0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void RadianceCascades::integrateFinalRadiance(Shader& integrationShader, float roughness) {
    integrationShader.use();
    integrationShader.setFloat("roughness", roughness);
    integrationShader.setInt("mergedRadiance", 0);
    
    // Bind merged cascade result
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mergedCascadeTexture);
    
    // Bind G-buffer for surface properties
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    integrationShader.setInt("gAlbedo", 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    integrationShader.setInt("gNormal", 2);
    
    // Render final integrated result
    glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[0]); // Use cascade 0 as final output
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    
    FullscreenQuad quad;
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Utility methods for band-limiting calculations

int RadianceCascades::calculateOptimalAngularResolution(int cascadeIndex, float roughness) const {
    // For rougher surfaces, we need more angular samples to capture the broader BRDF lobe
    // For smoother surfaces, fewer samples are sufficient
    float roughnessFactor = 1.0f + roughness * 2.0f; // Range: 1.0 to 3.0
    
    // Base angular resolution depends on cascade (near-field needs more detail)
    int baseResolution = cascadeAngularSamples[cascadeIndex];
    
    // Apply roughness scaling
    return int(baseResolution * roughnessFactor);
}

glm::ivec2 RadianceCascades::calculateOptimalSpatialResolution(int cascadeIndex) const {
    // Each cascade needs sufficient spatial resolution to capture its distance band
    // without aliasing. Near-field cascades need higher resolution.
    
    float scaleFactor = pow(spatialResolutionScaling, float(cascadeIndex));
    int width = int(screenWidth * scaleFactor);
    int height = int(screenHeight * scaleFactor);
    
    // Ensure minimum resolution
    width = std::max(width, 64);
    height = std::max(height, 64);
    
    return glm::ivec2(width, height);
}

glm::vec2 RadianceCascades::getCascadeDistanceRange(int cascadeIndex) const {
    if (cascadeIndex >= 0 && cascadeIndex < numCascades) {
        return glm::vec2(cascadeMinDistances[cascadeIndex], cascadeMaxDistances[cascadeIndex]);
    }
    return glm::vec2(0.0f, 1.0f);
}

void RadianceCascades::setBandLimitingParameters(int nearFieldAngularRes, int farFieldAngularRes, float spatialScaling) {
    nearFieldAngularSamples = nearFieldAngularRes;
    farFieldAngularSamples = farFieldAngularRes;
    spatialResolutionScaling = spatialScaling;
    
    // Recalculate parameters with new settings
    initializeBandLimitingParameters();
} 