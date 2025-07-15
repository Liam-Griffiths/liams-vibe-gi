// Scene.cpp
#include "../include/Scene.h"
#include "../include/TransformComponent.h"
#include "../include/MeshComponent.h"
#include "../include/LightComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene() : camera(glm::vec3(0.0f, 0.0f, 5.0f)) {
    // loadShadowTestScene(); // Changed to use the new shadow test scene
    loadDefaultLightbox();
}

void Scene::loadDefaultLightbox() {
    entities.clear();

    std::vector<Vertex> cubeVertices = {
        // positions          // normals (same as in main.cpp)
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},

        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},

        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},

        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},

        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},

        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)}
    };
    cubeMesh = std::make_unique<Mesh>(cubeVertices);

    // Floor
    auto floor = std::make_unique<Entity>();
    floor->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 0.1f, 5.0f)));
    floor->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(floor));

    // Ceiling
    auto ceiling = std::make_unique<Entity>();
    ceiling->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 0.1f, 5.0f)));
    ceiling->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(ceiling));

    // Left wall (red)
    auto leftWall = std::make_unique<Entity>();
    leftWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(-2.55f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 2.0f, 5.0f)));
    leftWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 0.0f, 0.0f)));
    entities.push_back(std::move(leftWall));

    // Right wall (green)
    auto rightWall = std::make_unique<Entity>();
    rightWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(2.55f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 2.0f, 5.0f)));
    rightWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.0f, 1.0f, 0.0f)));
    entities.push_back(std::move(rightWall));

    // Back wall
    auto backWall = std::make_unique<Entity>();
    backWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.0f, -2.55f), glm::vec3(0.0f), glm::vec3(5.0f, 2.0f, 0.1f)));
    backWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(backWall));

    // Short box
    auto shortBox = std::make_unique<Entity>();
    shortBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(1.0f, -0.75f, -1.0f), glm::vec3(0.0f, 18.0f, 0.0f), glm::vec3(1.0f, 0.5f, 1.0f)));
    shortBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(shortBox));

    // Tall box
    auto tallBox = std::make_unique<Entity>();
    tallBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(-1.0f, -0.25f, 1.0f), glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f)));
    tallBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(tallBox));

    // Light
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.9f, 0.0f)));
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 1.0f), 3.0f));
    entities.push_back(std::move(light));

    // Position camera inside the box
    camera.position = glm::vec3(0.0f, 0.0f, 4.0f);
    camera.yaw = -90.0f;
    camera.pitch = 0.0f;
    camera.updateCameraVectors();
}

void Scene::loadShadowTestScene() {
    entities.clear();

    std::vector<Vertex> cubeVertices = {
        // positions          // normals
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},

        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},

        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},

        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},

        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},

        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  1.0f,  0.0f)}
    };
    cubeMesh = std::make_unique<Mesh>(cubeVertices);

    // Large ground plane
    auto ground = std::make_unique<Entity>();
    ground->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(8.0f, 0.1f, 8.0f)));
    ground->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.8f, 0.8f, 0.8f))); // Light gray ground
    entities.push_back(std::move(ground));

    // Floating box (shadow caster)
    auto floatingBox = std::make_unique<Entity>();
    floatingBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(1.5f, 1.5f, 1.5f)));
    floatingBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.7f, 0.3f, 0.3f))); // Reddish box
    entities.push_back(std::move(floatingBox));

    // Additional smaller box for more shadows
    auto smallBox = std::make_unique<Entity>();
    smallBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(-2.0f, -0.5f, 1.0f), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(0.8f, 1.0f, 0.8f)));
    smallBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.3f, 0.7f, 0.3f))); // Greenish box
    entities.push_back(std::move(smallBox));

    // Another box for more interesting shadows
    auto tallBox = std::make_unique<Entity>();
    tallBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(2.5f, 0.0f, -1.5f), glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(1.0f, 2.5f, 1.0f)));
    tallBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.3f, 0.3f, 0.7f))); // Bluish box
    entities.push_back(std::move(tallBox));

    // Offset light source for dramatic shadows
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(3.0f, 4.0f, 2.0f))); // Offset light position
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 0.9f), 4.0f)); // Slightly warm white light
    entities.push_back(std::move(light));

    // Position camera for best shadow viewing
    camera.position = glm::vec3(-3.0f, 2.0f, 5.0f);
    camera.yaw = -45.0f;
    camera.pitch = -15.0f;
    camera.updateCameraVectors();
} 