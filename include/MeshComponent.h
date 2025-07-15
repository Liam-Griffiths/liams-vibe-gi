
// MeshComponent.h
#ifndef MESHCOMPONENT_H
#define MESHCOMPONENT_H

#include "Entity.h"
#include "../include/Mesh.h"

class MeshComponent : public Component {
public:
    Mesh* mesh;
    glm::vec3 color;

    MeshComponent(Mesh* m, glm::vec3 col = glm::vec3(1.0f)) : mesh(m), color(col) {}
};

#endif // MESHCOMPONENT_H 