
// LightComponent.h
#ifndef LIGHTCOMPONENT_H
#define LIGHTCOMPONENT_H

#include "Entity.h"
#include <glm/glm.hpp>

class LightComponent : public Component {
public:
    glm::vec3 color;
    float intensity;

    LightComponent(glm::vec3 col = glm::vec3(1.0f), float intens = 1.0f)
        : color(col), intensity(intens) {}
};

#endif // LIGHTCOMPONENT_H 