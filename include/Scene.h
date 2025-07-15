#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>

#include "Entity.h"
#include "Camera.h"
#include "Mesh.h"

class Scene {
public:
    std::vector<std::unique_ptr<Entity>> entities;
    Camera camera;
    std::unique_ptr<Mesh> cubeMesh;

    Scene();
    void loadDefaultLightbox();
    void loadShadowTestScene();
};

#endif 