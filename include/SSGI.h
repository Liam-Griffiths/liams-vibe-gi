#ifndef SSGI_H
#define SSGI_H

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>
#include "Shader.h"

class SSGI {
public:
    // G-Buffer textures
    unsigned int gPosition, gNormal, gAlbedo, gDepth;
    unsigned int gBuffer;
    
    // SSGI textures
    unsigned int ssgiTexture, ssgiBlurTexture;
    unsigned int ssgiFramebuffer, ssgiBlurFramebuffer;
    
    // Final composite framebuffer
    unsigned int finalFramebuffer, finalTexture;
    
    // Screen dimensions
    int screenWidth, screenHeight;
    
    // SSGI parameters
    float ssgiRadius;
    float ssgiIntensity;
    float ssgiMaxDistance;
    int ssgiSamples;
    
    SSGI(int width, int height);
    ~SSGI();
    
    // Setup functions
    void setupGBuffer();
    void setupSSGIFramebuffers();
    void setupFinalFramebuffer();
    
    // Rendering functions
    void bindGBufferForWriting();
    void bindSSGIForWriting();
    void bindSSGIBlurForWriting();
    void bindFinalForWriting();
    void bindForReading();
    
    // Utility functions
    void resizeBuffers(int newWidth, int newHeight);
    void debugOutput();
    
private:
    void createTexture(unsigned int& texture, GLenum internalFormat, GLenum format, GLenum type);
    void cleanup();
};

#endif // SSGI_H 