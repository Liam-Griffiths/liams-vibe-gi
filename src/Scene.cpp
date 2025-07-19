/**
 * Scene.cpp - Scene Management and Entity Setup Implementation
 * 
 * Implements the Scene class methods for managing entities and loading
 * different demo scenarios. The scene system uses the Entity-Component-System
 * (ECS) pattern to create flexible object composition.
 * 
 * Scene Loading Functions:
 * - loadTeapotLightbox(): Main demo scene with Utah teapot and PBR materials
 * - loadStoneFloorScene(): Demonstrates PBR textures on floor geometry
 * - loadShadowTestScene(): Multiple objects for shadow mapping tests
 * - loadDefaultLightbox(): Basic scene with simple geometry
 * 
 * Each scene configures:
 * - Geometry (meshes from OBJ files or procedural)
 * - Materials (PBR textures and properties)
 * - Lighting (point lights with position and properties)
 * - Camera (initial position and orientation)
 */

#include "../include/Scene.h"
#include "../include/TransformComponent.h"
#include "../include/MeshComponent.h"
#include "../include/MaterialComponent.h"
#include "../include/LightComponent.h"
#include "../scripts/Behaviour.h"
#include "../scripts/RotationComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

/**
 * Scene Constructor - Initialize scene with default content
 * 
 * Creates the main scene and loads the default teapot lightbox demo.
 * Sets up camera at a good viewing position for the demo content.
 */
Scene::Scene() : camera(glm::vec3(0.0f, 0.0f, 5.0f)) {
    // Load the main demo scene - teapot with PBR materials in lightbox setup
    loadTeapotLightbox();
    
    // Alternative scenes available for different testing scenarios:
    // loadStoneFloorScene();   // PBR material testing with detailed textures
    // loadShadowTestScene();   // Shadow mapping testing with multiple objects
    // loadDefaultLightbox();   // Basic geometry for simple lighting tests
}

/**
 * Load Default Lightbox Scene
 * 
 * Creates a simple scene with basic cube geometry for testing fundamental
 * lighting and rendering functionality. Good starting point for debugging
 * rendering issues or testing new features.
 */
void Scene::loadDefaultLightbox() {
    // Clear any existing entities
    entities.clear();

    // Define cube geometry with positions and normals
    // Using triangle list format for maximum compatibility
    std::vector<Vertex> cubeVertices = {
        // Back face (facing negative Z)
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f,  0.0f, -1.0f)},

        // Front face (facing positive Z)
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f,  0.0f, 1.0f)},

        // Left face (facing negative X)
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f, 0.0f,  0.0f)},

        // Right face (facing positive X)
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},
        {glm::vec3(0.5f,  0.5f,  0.5f),   glm::vec3(1.0f,  0.0f,  0.0f)},

        // Bottom face (facing negative Y)
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f, -0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(0.5f, -0.5f,  0.5f),   glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},
        {glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(0.0f, -1.0f,  0.0f)},

        // Top face (facing positive Y)
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
    floor->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(10.0f, 0.1f, 10.0f)));
    floor->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(floor));

    // Ceiling
    auto ceiling = std::make_unique<Entity>();
    ceiling->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(10.0f, 0.1f, 10.0f)));
    ceiling->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(ceiling));

    // Left wall (red)
    auto leftWall = std::make_unique<Entity>();
    leftWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(-5.1f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 4.0f, 10.0f)));
    leftWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.0f, 1.0f, 0.0f)));
    entities.push_back(std::move(leftWall));

    // Right wall (green)
    auto rightWall = std::make_unique<Entity>();
    rightWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(5.1f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 4.0f, 10.0f)));
    rightWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 0.0f, 0.0f)));
    entities.push_back(std::move(rightWall));

    // Back wall
    auto backWall = std::make_unique<Entity>();
    backWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.0f, -5.1f), glm::vec3(0.0f), glm::vec3(10.0f, 4.0f, 0.1f)));
    backWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(backWall));

    // Short box
    auto shortBox = std::make_unique<Entity>();
    shortBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(2.0f, -1.5f, -2.0f), glm::vec3(0.0f, 18.0f, 0.0f), glm::vec3(2.0f, 1.0f, 2.0f)));
    shortBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(shortBox));

    // Tall box
    auto tallBox = std::make_unique<Entity>();
    tallBox->addComponent(std::make_unique<TransformComponent>(glm::vec3(-2.0f, -0.5f, 2.0f), glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(2.0f, 3.0f, 2.0f)));
    tallBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(tallBox));

    // Light
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.0f, 0.0f)));
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 1.0f), 3.0f));
    light->addComponent(std::make_unique<MeshComponent>(Mesh::createSphere(0.1f, 20, 20).get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(light));

    // Position camera inside the box
    camera.position = glm::vec3(0.0f, 0.0f, 8.0f);
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
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 0.9f), 4.0f, 5.0f)); // Larger radius for softer light
    entities.push_back(std::move(light));

    // Position camera for best shadow viewing (moved back for better view)
    camera.position = glm::vec3(-4.0f, 2.5f, 8.0f);
    camera.yaw = -45.0f;
    camera.pitch = -15.0f;
    camera.updateCameraVectors();
}

void Scene::loadTeapotLightbox() {
    entities.clear();

    std::cout << "Loading teapot lightbox scene..." << std::endl;

    // Load the teapot mesh
    std::cout << "Attempting to load teapot model from: models/teapot.obj" << std::endl;
    teapotMesh = Mesh::loadFromOBJ("models/teapot.obj");
    if (!teapotMesh) {
        std::cerr << "Failed to load teapot model, falling back to cube" << std::endl;
        // Fallback to a simple cube if teapot loading fails
        std::vector<Vertex> cubeVertices = {
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
        teapotMesh = std::make_unique<Mesh>(cubeVertices);
    }

    // Load the bunny mesh
    std::cout << "Attempting to load bunny model from: models/bunny.obj" << std::endl;
    bunnyMesh = Mesh::loadFromOBJ("models/bunny.obj");
    if (!bunnyMesh) {
        std::cerr << "Failed to load bunny model, falling back to cube" << std::endl;
        // Fallback to a simple cube if bunny loading fails
        std::vector<Vertex> cubeVertices = {
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
        bunnyMesh = std::make_unique<Mesh>(cubeVertices);
    }

    // Load the dragon mesh
    std::cout << "Attempting to load dragon model from: models/dragon.obj" << std::endl;
    dragonMesh = Mesh::loadFromOBJ("models/dragon.obj");
    if (!dragonMesh) {
        std::cerr << "Failed to load dragon model, falling back to cube" << std::endl;
        // Fallback to a simple cube if dragon loading fails
        std::vector<Vertex> cubeVertices = {
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
        dragonMesh = std::make_unique<Mesh>(cubeVertices);
    }

    // Create cube mesh for walls
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

    // Floor with stone texture - larger lightbox
    floorMesh = Mesh::createPlane(25.0f, 25.0f, 8, 8); // Much larger floor for bigger lightbox
    auto floor = std::make_unique<Entity>();
    floor->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f))); // No rotation - plane should be horizontal by default
    floor->addComponent(std::make_unique<MeshComponent>(floorMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f))); // White base color, will be overridden by material
    auto stoneMaterial = MaterialComponent::createPBR("stone", glm::vec2(5.0f, 5.0f), 0.025f); // More tiling for larger floor
    floor->addComponent(std::move(stoneMaterial));
    entities.push_back(std::move(floor));

    // Ceiling (light gray) - much larger and higher
    auto ceiling = std::make_unique<Entity>();
    ceiling->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(25.0f, 0.1f, 25.0f))); // Much bigger and higher
    ceiling->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.9f, 0.9f, 0.9f)));
    entities.push_back(std::move(ceiling));

    // Left wall (green) - much taller and longer
    auto leftWall = std::make_unique<Entity>();
    leftWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(-12.6f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 8.0f, 25.0f))); // Much taller and longer
    leftWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.12f, 0.45f, 0.15f))); // Classic green
    entities.push_back(std::move(leftWall));

    // Right wall (red) - much taller and longer
    auto rightWall = std::make_unique<Entity>();
    rightWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(12.6f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 8.0f, 25.0f))); // Much taller and longer
    rightWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.7f, 0.12f, 0.15f))); // Classic red
    entities.push_back(std::move(rightWall));

    // Back wall (white) - much taller and wider
    auto backWall = std::make_unique<Entity>();
    backWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 1.0f, -12.6f), glm::vec3(0.0f), glm::vec3(25.0f, 8.0f, 0.1f))); // Much taller and wider
    backWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.9f, 0.9f, 0.9f)));
    entities.push_back(std::move(backWall));

    // Center teapot (main subject) - larger scale for bigger room
    auto centerTeapot = std::make_unique<Entity>();
    centerTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -1.8f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.2f, 1.2f, 1.2f))); // Larger scale
    centerTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.7f, 0.7f, 0.9f))); // Light blue
    entities.push_back(std::move(centerTeapot));

    // Left bunny (yellow, scaled up 2x larger)
    auto leftBunny = std::make_unique<Entity>();
    leftBunny->addComponent(std::make_unique<TransformComponent>(glm::vec3(-3.5f, -2.0f, -1.5f), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(1.8f, 1.8f, 1.8f))); // 2x larger than before
    leftBunny->addComponent(std::make_unique<MeshComponent>(bunnyMesh.get(), glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow
    entities.push_back(std::move(leftBunny));

    // Right teapot (scaled up for bigger room)
    auto rightTeapot = std::make_unique<Entity>();
    rightTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(3.5f, -2.0f, 1.5f), glm::vec3(0.0f, -45.0f, 0.0f), glm::vec3(0.9f, 0.9f, 0.9f))); // Larger
    rightTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.3f, 0.8f, 0.4f))); // Green
    entities.push_back(std::move(rightTeapot));

    // Back teapot (larger for bigger room)
    auto backTeapot = std::make_unique<Entity>();
    backTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -2.2f, -3.5f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(0.7f, 0.7f, 0.7f))); // Larger
    backTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.8f, 0.3f, 0.8f))); // Purple
    entities.push_back(std::move(backTeapot));

    // Tiny green dragon with rotation behavior
    auto dragon = std::make_unique<Entity>();
    dragon->addComponent(std::make_unique<TransformComponent>(glm::vec3(6.0f, -1.5f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.03f, 0.03f, 0.03f))); // Tiny scale to fit properly in room
    dragon->addComponent(std::make_unique<MeshComponent>(dragonMesh.get(), glm::vec3(0.4f, 0.8f, 0.2f))); // Green dragon
    dragon->addComponent(std::make_unique<RotationComponent>(20.0f, glm::vec3(0.0f, 1.0f, 0.0f))); // Rotate slowly around Y-axis at 20 degrees/second
    entities.push_back(std::move(dragon));

    // Emissive blue cube - glowing light source
    auto emissiveCube = std::make_unique<Entity>();
    emissiveCube->addComponent(std::make_unique<TransformComponent>(glm::vec3(-5.0f, 0.5f, 4.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f))); // Position and slight rotation
    emissiveCube->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f))); // White base color (will be overridden by material)
    auto emissiveMaterial = MaterialComponent::createEmissive(
        glm::vec3(0.3f, 0.3f, 0.8f),   // Base color (darker blue, less overwhelming)
        glm::vec3(1.5f, 4.0f, 10.0f),  // Emission (higher values for stronger GI, dimmer surface display)
        0.1f,                          // Low roughness (slightly shiny)
        0.0f                           // Non-metallic
    );
    emissiveCube->addComponent(std::move(emissiveMaterial));
    entities.push_back(std::move(emissiveCube));

    // Large bright white ceiling light bar for general illumination
    auto ceilingLightBar = std::make_unique<Entity>();
    ceilingLightBar->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, 4.0f, 0.0f),        // Ceiling position
        glm::vec3(0.0f, 0.0f, 0.0f),        // No rotation
        glm::vec3(6.0f, 0.15f, 1.5f)        // Long bar shape (6x0.15x1.5)
    ));
    ceilingLightBar->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f))); // White base color
    auto ceilingLightMaterial = MaterialComponent::createEmissive(
        glm::vec3(0.98f, 0.98f, 0.98f),     // Very bright white base color
        glm::vec3(50.0f, 50.0f, 50.0f),     // Extremely strong white emission for long-range room lighting
        0.0f,                               // Very rough (non-reflective surface)
        0.0f                                // Non-metallic
    );
    ceilingLightBar->addComponent(std::move(ceilingLightMaterial));
    entities.push_back(std::move(ceilingLightBar));

    // Powerful overhead light source for large room
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 2.2f, 0.0f))); // Higher position
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 0.95f), 12.0f, 8.0f)); // Large radius for very soft light
    light->addComponent(std::make_unique<MeshComponent>(Mesh::createSphere(0.15f, 20, 20).get(), glm::vec3(1.0f, 1.0f, 1.0f))); // Larger sphere
    entities.push_back(std::move(light));

    // Position camera for optimal viewing of larger lightbox with all objects
    camera.position = glm::vec3(0.0f, 2.0f, 18.0f); // Much farther back and higher for larger room
    camera.yaw = -90.0f;
    camera.pitch = -5.0f; // Slight downward angle to see floor better
    camera.updateCameraVectors();
}

void Scene::loadStoneFloorScene() {
    entities.clear();

    std::cout << "Loading stone floor scene with PBR materials..." << std::endl;

    // Load the teapot mesh for test objects
    teapotMesh = Mesh::loadFromOBJ("models/teapot.obj");
    if (!teapotMesh) {
        std::cerr << "Failed to load teapot model, using cube instead" << std::endl;
        teapotMesh = Mesh::createCube();
    }

    // Create a large tiled stone floor using plane mesh
    floorMesh = Mesh::createPlane(20.0f, 20.0f, 10, 10); // Large floor with many segments for tiling
    
    // Create the stone material with proper tiling (4x4 repeats) and enhanced height scale
    auto stoneMaterial = MaterialComponent::createPBR("stone", glm::vec2(4.0f, 4.0f), 0.03f);
    
    // Create floor entity with stone material
    auto floor = std::make_unique<Entity>();
    floor->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
    floor->addComponent(std::make_unique<MeshComponent>(floorMesh.get(), glm::vec3(1.0f))); // White base color, will be overridden by material
    floor->addComponent(std::move(stoneMaterial));
    entities.push_back(std::move(floor));

    // Add test objects on the floor
    
    // Central teapot
    auto centerTeapot = std::make_unique<Entity>();
    centerTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.5f, 1.5f, 1.5f)));
    centerTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.8f, 0.2f, 0.2f))); // Red teapot
    entities.push_back(std::move(centerTeapot));

    // Left teapot (blue, metallic)
    auto leftTeapot = std::make_unique<Entity>();
    leftTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(-4.0f, -1.0f, -2.0f), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(1.2f, 1.2f, 1.2f)));
    leftTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.2f, 0.4f, 0.8f))); // Blue teapot
    auto metallicMaterial = MaterialComponent::createSolid(glm::vec3(0.2f, 0.4f, 0.8f), 0.1f, 0.8f); // Low roughness, high metallic
    leftTeapot->addComponent(std::move(metallicMaterial));
    entities.push_back(std::move(leftTeapot));

    // Right teapot (green, rough)
    auto rightTeapot = std::make_unique<Entity>();
    rightTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(4.0f, -1.0f, 2.0f), glm::vec3(0.0f, -45.0f, 0.0f), glm::vec3(1.2f, 1.2f, 1.2f)));
    rightTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.2f, 0.8f, 0.3f))); // Green teapot
    auto roughMaterial = MaterialComponent::createSolid(glm::vec3(0.2f, 0.8f, 0.3f), 0.9f, 0.0f); // High roughness, no metallic
    rightTeapot->addComponent(std::move(roughMaterial));
    entities.push_back(std::move(rightTeapot));

    // Back teapot (gold, medium properties)
    auto backTeapot = std::make_unique<Entity>();
    backTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -1.2f, -4.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
    backTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(1.0f, 0.8f, 0.2f))); // Gold teapot
    auto goldMaterial = MaterialComponent::createSolid(glm::vec3(1.0f, 0.8f, 0.2f), 0.3f, 0.7f); // Medium roughness, high metallic
    backTeapot->addComponent(std::move(goldMaterial));
    entities.push_back(std::move(backTeapot));

    // Add some cubes for variety
    cubeMesh = Mesh::createCube();
    
    // Cube 1 - Rough plastic
    auto cube1 = std::make_unique<Entity>();
    cube1->addComponent(std::make_unique<TransformComponent>(glm::vec3(-2.0f, -1.5f, 3.0f), glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
    cube1->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.9f, 0.1f, 0.9f))); // Purple
    auto plasticMaterial = MaterialComponent::createSolid(glm::vec3(0.9f, 0.1f, 0.9f), 0.8f, 0.0f);
    cube1->addComponent(std::move(plasticMaterial));
    entities.push_back(std::move(cube1));
    
    // Cube 2 - Smooth metal
    auto cube2 = std::make_unique<Entity>();
    cube2->addComponent(std::make_unique<TransformComponent>(glm::vec3(2.5f, -1.5f, -3.5f), glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
    cube2->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.7f, 0.7f, 0.8f))); // Silver
    auto metalMaterial = MaterialComponent::createSolid(glm::vec3(0.7f, 0.7f, 0.8f), 0.05f, 0.95f);
    cube2->addComponent(std::move(metalMaterial));
    entities.push_back(std::move(cube2));

    // Strong overhead light to show off the materials
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 3.0f, 0.0f)));
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 0.95f), 15.0f, 10.0f)); // Strong, slightly warm light
    auto lightMesh = Mesh::createSphere(0.2f, 20, 20);
    light->addComponent(std::make_unique<MeshComponent>(lightMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f)));
    entities.push_back(std::move(light));

    // Position camera for good overview of the scene
    camera.position = glm::vec3(0.0f, 2.0f, 8.0f);
    camera.yaw = -90.0f;
    camera.pitch = -15.0f; // Look down slightly
    camera.updateCameraVectors();
    
    std::cout << "Stone floor scene loaded with " << entities.size() << " entities." << std::endl;
} 