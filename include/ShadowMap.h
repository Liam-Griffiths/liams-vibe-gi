// ShadowMap.h
#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <glm/glm.hpp>

class ShadowMap {
public:
    static const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    
    unsigned int depthMapFBO;
    unsigned int depthMap;
    
    ShadowMap();
    ~ShadowMap();
    
    void bindForWriting();
    void bindForReading(unsigned int textureUnit);
    glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos);
    
private:
    void setupShadowMap();
};

#endif // SHADOWMAP_H 