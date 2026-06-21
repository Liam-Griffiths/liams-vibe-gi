// ShadowMap.cpp
#include "../include/ShadowMap.h"
#include <GLFW/glfw3.h>
#include "../include/GLHeaders.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

ShadowMap::ShadowMap() {
    setupShadowMap();
}

ShadowMap::~ShadowMap() {
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
}

void ShadowMap::setupShadowMap() {
    glGenFramebuffers(1, &depthMapFBO);
    
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::SHADOWMAP::FRAMEBUFFER_NOT_COMPLETE" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindForWriting() {
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::bindForReading(unsigned int textureUnit) {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthMap);
}

glm::mat4 ShadowMap::getLightSpaceMatrix(const glm::vec3& lightPos, float lightRadius, float orthoSize) {
    glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);

    // Orthographic bounds. An explicit orthoSize (from the GUI "Shadow Coverage" control)
    // lets the frustum be sized to the scene so the 4096^2 map isn't wasted on empty space
    // (the main cause of blocky shadows). 0 falls back to the old light-radius heuristic.
    float projectionSize = (orthoSize > 0.0f) ? orthoSize : (15.0f + lightRadius * 3.0f);

    // Fit the depth range to the actual light->scene distance. The old fixed far_plane=25
    // clipped large scenes (light high above Sponza), which both lost shadows and wasted
    // depth precision -> jagged/incomplete shadows.
    float dist = glm::length(lightPos - sceneCenter);
    float near_plane = std::max(0.5f, dist * 0.05f);
    float far_plane = dist + projectionSize + 5.0f;

    glm::mat4 lightProjection = glm::ortho(-projectionSize, projectionSize, -projectionSize, projectionSize, near_plane, far_plane);

    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    return lightSpaceMatrix;
} 