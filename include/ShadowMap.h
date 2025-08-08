// ShadowMap.h
#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <glm/glm.hpp>

class ShadowMap {
public:
    static const unsigned int SHADOW_WIDTH = 4096;   // High quality shadows (was 16384)
    static const unsigned int SHADOW_HEIGHT = 4096;  // High quality shadows (was 16384)
    
    unsigned int depthMapFBO;
    unsigned int depthMap;
    
    ShadowMap();
    ~ShadowMap();
    
    void bindForWriting();
    void bindForReading(unsigned int textureUnit);
    glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos, float lightRadius = 2.0f);
    
private:
    void setupShadowMap();
};

#endif // SHADOWMAP_H 