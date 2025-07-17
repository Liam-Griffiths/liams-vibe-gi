#include "../include/RadianceCascades.h"
#include <OpenGL/gl3.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <random>
#include <cstdlib>
#include "../include/FullscreenQuad.h"

// Remove global variables that conflict with member variables

RadianceCascades::RadianceCascades(int width, int height, int num, float baseSpacing, float angularBase) : screenWidth(width), screenHeight(height), numCascades(num), probeSpacing(baseSpacing), angularResolution(angularBase), rboDepth(0), useTemporalBuffer(true), frameCounter(0), historyTexture(0) {
    setupGBuffer();
    setupCascades();
    setupBlurTargets();
    setupTemporalBuffers();
    setupTAA(); // Add TAA setup
    setupSSAO(); // Add SSAO setup
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
        // Start at FULL resolution for better quality, then downsample more gradually
        int res_x = std::max(128, screenWidth >> i); // Start at full resolution, min 128px
        int res_y = std::max(128, screenHeight >> i);
        cascadeWidths[i] = res_x;
        cascadeHeights[i] = res_y;

        glBindTexture(GL_TEXTURE_2D, cascadeTextures[i]);
        // Use 16-bit precision for major bandwidth savings (75% less memory!)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, res_x, res_y, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
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

    // Normal buffer (16-bit is more than enough for normals)
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_HALF_FLOAT, NULL);
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

    // Depth renderbuffer (24-bit is standard and efficient)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[5] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
    glDrawBuffers(5, attachments);

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

void RadianceCascades::blur(Shader& blurShader) {
    blurShader.use();
    FullscreenQuad quad;
    
    // Apply aggressive blur to ALL cascades for consistent smoothing
    for (int i = 0; i < numCascades; ++i) {
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
}

void RadianceCascades::cleanup() {
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedo);
    glDeleteTextures(1, &gDepth);
    glDeleteTextures(1, &gVelocity); // Added gVelocity cleanup
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteFramebuffers(numCascades, cascadeFBOs.data());
    glDeleteTextures(numCascades, cascadeTextures.data());
    glDeleteFramebuffers(numCascades, tempBlurFBOs.data());
    glDeleteTextures(numCascades, tempBlurTextures.data());
    glDeleteFramebuffers(numCascades, temporalFBOs.data());
    glDeleteTextures(numCascades, temporalTextures.data());
    glDeleteTextures(1, &historyTexture);
    
    // SSAO cleanup
    glDeleteFramebuffers(1, &ssaoFBO);
    glDeleteFramebuffers(1, &ssaoBlurFBO);
    glDeleteTextures(1, &ssaoTexture);
    glDeleteTextures(1, &ssaoBlurTexture);
    glDeleteTextures(1, &noiseTexture);
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
}

void RadianceCascades::compute(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setFloat("time", glfwGetTime());
    shader.setInt("frameCounter", frameCounter);
    shader.setBool("useTemporalAccumulation", useTemporalBuffer && frameCounter > 0);
    shader.setInt("gPosition", 0);
    shader.setInt("gNormal", 1);
    shader.setInt("gAlbedo", 2);
    shader.setInt("gLinearDepth", 3);
    
    bindForReading();
    FullscreenQuad quad;
    
    for (int i = numCascades - 1; i >= 0; --i) {
        int res_x = cascadeWidths[i];
        int res_y = cascadeHeights[i];
        
        // Render to current cascade
        glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
        glViewport(0, 0, res_x, res_y);
        glClear(GL_COLOR_BUFFER_BIT);
        
        shader.setInt("cascadeIndex", i);
        shader.setFloat("minDist", pow(2.0f, float(i)));
        shader.setFloat("maxDist", pow(2.0f, float(i + 1)));
        
        // Bind previous cascade (spatial hierarchy)
        if (i < numCascades - 1) {
            shader.setInt("previousCascade", 4);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, cascadeTextures[i+1]);
        }
        
        // Bind temporal buffer (temporal accumulation)
        if (useTemporalBuffer && frameCounter > 0) {
            shader.setInt("temporalBuffer", 5);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, temporalTextures[i]);
        } else {
            // Ensure we don't use uninitialized temporal data
            shader.setBool("useTemporalAccumulation", false);
        }
        
        quad.render();
        
        // Copy result to temporal buffer for next frame
        if (useTemporalBuffer) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, cascadeFBOs[i]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temporalFBOs[i]);
            glBlitFramebuffer(0, 0, res_x, res_y, 0, 0, res_x, res_y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }
    
    frameCounter++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void RadianceCascades::resetTemporalAccumulation() {
    frameCounter = 0;
    
    // Clear all temporal buffers
    for (int i = 0; i < numCascades; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, temporalFBOs[i]);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    
    // Generate 64 sample points in hemisphere
    for (int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            ((float)rand() / RAND_MAX) * 2.0f - 1.0f,
            ((float)rand() / RAND_MAX) * 2.0f - 1.0f,
            (float)rand() / RAND_MAX  // Keep samples in positive Z hemisphere
        );
        
        sample = normalize(sample);
        sample *= (float)rand() / RAND_MAX;
        
        // Bias samples toward origin for better results
        float scale = (float)i / 64.0f;
        scale = 0.1f + scale * scale * (1.0f - 0.1f); // Lerp between 0.1 and 1.0
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
    for (int i = 0; i < 64; ++i) {
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
    
    FullscreenQuad quad;
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadianceCascades::blurSSAO(Shader& blurShader) {
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    
    blurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    blurShader.setInt("ssaoInput", 0);
    
    FullscreenQuad quad;
    quad.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
} 