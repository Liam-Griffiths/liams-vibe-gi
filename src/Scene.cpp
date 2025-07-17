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

    // Floor with stone texture - lowered and bigger
    floorMesh = Mesh::createPlane(15.0f, 15.0f, 8, 8); // Large floor with segments for tiling
    auto floor = std::make_unique<Entity>();
    floor->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f))); // No rotation - plane should be horizontal by default
    floor->addComponent(std::make_unique<MeshComponent>(floorMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f))); // White base color, will be overridden by material
    auto stoneMaterial = MaterialComponent::createPBR("stone"); // Load stone textures
    floor->addComponent(std::move(stoneMaterial));
    entities.push_back(std::move(floor));

    // Ceiling (light gray) - raised and bigger
    auto ceiling = std::make_unique<Entity>();
    ceiling->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(15.0f, 0.1f, 15.0f))); // Bigger and higher
    ceiling->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.9f, 0.9f, 0.9f)));
    entities.push_back(std::move(ceiling));

    // Left wall (green) - taller and longer
    auto leftWall = std::make_unique<Entity>();
    leftWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(-7.6f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 6.0f, 15.0f))); // Taller (6.0f) and longer
    leftWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.12f, 0.45f, 0.15f))); // Classic green
    entities.push_back(std::move(leftWall));

    // Right wall (red) - taller and longer
    auto rightWall = std::make_unique<Entity>();
    rightWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(7.6f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f, 6.0f, 15.0f))); // Taller (6.0f) and longer
    rightWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.7f, 0.12f, 0.15f))); // Classic red
    entities.push_back(std::move(rightWall));

    // Back wall (white) - taller and wider
    auto backWall = std::make_unique<Entity>();
    backWall->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 0.0f, -7.6f), glm::vec3(0.0f), glm::vec3(15.0f, 6.0f, 0.1f))); // Taller (6.0f) and wider
    backWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.9f, 0.9f, 0.9f)));
    entities.push_back(std::move(backWall));

    // Center teapot (main subject) - larger scale for bigger room
    auto centerTeapot = std::make_unique<Entity>();
    centerTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, -1.8f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.2f, 1.2f, 1.2f))); // Larger scale
    centerTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.7f, 0.7f, 0.9f))); // Light blue
    entities.push_back(std::move(centerTeapot));

    // Left teapot (scaled up for bigger room)
    auto leftTeapot = std::make_unique<Entity>();
    leftTeapot->addComponent(std::make_unique<TransformComponent>(glm::vec3(-3.5f, -2.0f, -1.5f), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(0.9f, 0.9f, 0.9f))); // Larger
    leftTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.9f, 0.6f, 0.3f))); // Orange
    entities.push_back(std::move(leftTeapot));

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

    // Powerful overhead light source for large room
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f, 2.2f, 0.0f))); // Higher position
    light->addComponent(std::make_unique<LightComponent>(glm::vec3(1.0f, 1.0f, 0.95f), 12.0f, 8.0f)); // Large radius for very soft light
    light->addComponent(std::make_unique<MeshComponent>(Mesh::createSphere(0.15f, 20, 20).get(), glm::vec3(1.0f, 1.0f, 1.0f))); // Larger sphere
    entities.push_back(std::move(light));

    // Position camera for optimal teapot viewing
    camera.position = glm::vec3(0.0f, 0.0f, 12.0f); // Farther back for larger room
    camera.yaw = -90.0f;
    camera.pitch = 0.0f;
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
    
    // Create the stone material
    auto stoneMaterial = MaterialComponent::createPBR("stone");
    
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