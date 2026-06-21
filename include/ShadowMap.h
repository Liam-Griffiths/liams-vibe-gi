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
    // orthoSize is the half-extent of the shadow frustum in world units. Sizing it to the
    // scene is what keeps shadows crisp: too large wastes texels (blocky shadows). Pass 0
    // to fall back to the automatic light-radius heuristic.
    glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos, float lightRadius = 2.0f, float orthoSize = 0.0f);
    
private:
    void setupShadowMap();
};

#endif // SHADOWMAP_H 