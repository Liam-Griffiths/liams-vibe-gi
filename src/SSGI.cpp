#include "../include/SSGI.h"
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include <iostream>

SSGI::SSGI(int width, int height) 
    : screenWidth(width), screenHeight(height),
      ssgiRadius(2.0f), ssgiIntensity(3.0f), ssgiMaxDistance(2.0f), ssgiSamples(16) {
    
    setupGBuffer();
    setupSSGIFramebuffers();
    setupFinalFramebuffer();
}

SSGI::~SSGI() {
    cleanup();
}

void SSGI::setupGBuffer() {
    // Generate G-Buffer
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    
    // Position buffer (RGB16F for world positions)
    createTexture(gPosition, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    
    // Normal buffer (RGB16F for world normals)
    createTexture(gNormal, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    
    // Albedo buffer (RGBA for color + metallic/roughness)
    createTexture(gAlbedo, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    
    // Depth buffer
    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);
    
    // Set which color attachments to use
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SSGI::G_BUFFER_NOT_COMPLETE" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSGI::setupSSGIFramebuffers() {
    // SSGI main framebuffer
    glGenFramebuffers(1, &ssgiFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ssgiFramebuffer);
    
    createTexture(ssgiTexture, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssgiTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SSGI::SSGI_FRAMEBUFFER_NOT_COMPLETE" << std::endl;
    }
    
    // SSGI blur framebuffer
    glGenFramebuffers(1, &ssgiBlurFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, ssgiBlurFramebuffer);
    
    createTexture(ssgiBlurTexture, GL_RGB16F, GL_RGB, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssgiBlurTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SSGI::SSGI_BLUR_FRAMEBUFFER_NOT_COMPLETE" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSGI::setupFinalFramebuffer() {
    // Final composite framebuffer
    glGenFramebuffers(1, &finalFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);
    
    createTexture(finalTexture, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SSGI::FINAL_FRAMEBUFFER_NOT_COMPLETE" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSGI::bindGBufferForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SSGI::bindSSGIForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, ssgiFramebuffer);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SSGI::bindSSGIBlurForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, ssgiBlurFramebuffer);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SSGI::bindFinalForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SSGI::bindForReading() {
    // Bind G-Buffer textures for reading
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ssgiTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, ssgiBlurTexture);
}

void SSGI::resizeBuffers(int newWidth, int newHeight) {
    if (newWidth == screenWidth && newHeight == screenHeight) {
        return;
    }
    
    screenWidth = newWidth;
    screenHeight = newHeight;
    
    cleanup();
    setupGBuffer();
    setupSSGIFramebuffers();
    setupFinalFramebuffer();
}

void SSGI::debugOutput() {
    std::cout << "SSGI Debug Info:" << std::endl;
    std::cout << "  Screen Size: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << "  SSGI Radius: " << ssgiRadius << std::endl;
    std::cout << "  SSGI Intensity: " << ssgiIntensity << std::endl;
    std::cout << "  SSGI Samples: " << ssgiSamples << std::endl;
}

void SSGI::createTexture(unsigned int& texture, GLenum internalFormat, GLenum format, GLenum type) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, screenWidth, screenHeight, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SSGI::cleanup() {
    // Delete G-Buffer
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedo);
    glDeleteTextures(1, &gDepth);
    glDeleteFramebuffers(1, &gBuffer);
    
    // Delete SSGI framebuffers
    glDeleteTextures(1, &ssgiTexture);
    glDeleteTextures(1, &ssgiBlurTexture);
    glDeleteFramebuffers(1, &ssgiFramebuffer);
    glDeleteFramebuffers(1, &ssgiBlurFramebuffer);
    
    // Delete final framebuffer
    glDeleteTextures(1, &finalTexture);
    glDeleteFramebuffers(1, &finalFramebuffer);
} 