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
Scene::Scene() : camera(glm::vec3(0.0f, 0.0f, 5.0f)), currentScene(GLTF_SPONZA) {
    // Default to the glTF Sponza PBR scene (run scripts/fetch_sponza.sh to get the asset).
    loadScene(GLTF_SPONZA);

    // Alternative scenes available for different testing scenarios:
    // loadCornellBox();        // Classic Cornell box test scene
    // loadTeapotLightbox();    // Teapot with PBR materials in lightbox setup
    // loadStoneFloorScene();   // PBR material testing with detailed textures
    // loadShadowTestScene();   // Shadow mapping testing with multiple objects
    // loadDefaultLightbox();   // Basic geometry for simple lighting tests
    // loadSponzaScene();       // Large architectural scene with overhead lighting
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

/**
 * Load Traditional Cornell Box Scene
 * 
 * Creates the classic Cornell box - a simple room used in computer graphics
 * to test rendering algorithms, especially global illumination. The traditional
 * Cornell box consists of:
 * - White floor, ceiling, and back wall
 * - Red left wall and green right wall  
 * - A bright light source on the ceiling
 * - Two boxes inside (one short, one tall)
 * - Camera positioned to look into the box
 * 
 * This scene is perfect for testing:
 * - Color bleeding from colored walls
 * - Shadow casting and soft shadows
 * - Global illumination algorithms
 * - Light transport and indirect lighting
 */
void Scene::loadCornellBox() {
    // Clear any existing entities
    entities.clear();

    std::cout << "Loading traditional Cornell box scene..." << std::endl;

    // Load the teapot mesh for the spinning teapot
    std::cout << "Attempting to load teapot model from: models/teapot.obj" << std::endl;
    teapotMesh = Mesh::loadFromOBJ("models/teapot.obj");
    if (!teapotMesh) {
        std::cerr << "Failed to load teapot model, falling back to cube" << std::endl;
        // Fallback to a simple cube if teapot loading fails
        std::vector<Vertex> fallbackVertices = {
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
        teapotMesh = std::make_unique<Mesh>(fallbackVertices);
    }

    // Create cube geometry for all box elements
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

    // Cornell Box Room Dimensions (classic proportions)
    // The box is 5 units wide, 5 units tall, 5.5 units deep
    const float roomWidth = 5.0f;
    const float roomHeight = 5.0f;
    const float roomDepth = 5.5f;
    const float wallThickness = 0.1f;

    // FLOOR - Glossy reflective surface. Low roughness so screen-space reflections (SSR,
    // toggle in the GUI) kick in: SSR reflects any surface with roughness <= 0.8, scaled by
    // (1 - roughness), so a near-mirror floor mirrors the walls/teapot above it.
    auto floor = std::make_unique<Entity>();
    floor->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, -roomHeight/2.0f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(roomWidth, wallThickness, roomDepth)
    ));
    floor->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    floor->addComponent(MaterialComponent::createSolid(glm::vec3(0.73f, 0.73f, 0.73f), 0.08f, 0.0f));
    entities.push_back(std::move(floor));

    // CEILING WITH SQUARE SKYLIGHT - Four separate panels leaving a square hole in the center
    const float holeSize = 1.5f; // Size of the square skylight opening
    const float panelWidth = (roomWidth - holeSize) / 2.0f; // Width of each side panel
    const float panelDepth = (roomDepth - holeSize) / 2.0f; // Depth of each front/back panel
    
    // Front ceiling panel
    auto ceilingFront = std::make_unique<Entity>();
    ceilingFront->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, roomHeight/2.0f, holeSize/2.0f + panelDepth/2.0f), 
        glm::vec3(0.0f), 
        glm::vec3(roomWidth, wallThickness, panelDepth)
    ));
    ceilingFront->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    entities.push_back(std::move(ceilingFront));
    
    // Back ceiling panel
    auto ceilingBack = std::make_unique<Entity>();
    ceilingBack->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, roomHeight/2.0f, -holeSize/2.0f - panelDepth/2.0f), 
        glm::vec3(0.0f), 
        glm::vec3(roomWidth, wallThickness, panelDepth)
    ));
    ceilingBack->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    entities.push_back(std::move(ceilingBack));
    
    // Left ceiling panel  
    auto ceilingLeft = std::make_unique<Entity>();
    ceilingLeft->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(-holeSize/2.0f - panelWidth/2.0f, roomHeight/2.0f, 0.0f), 
        glm::vec3(0.0f), 
        glm::vec3(panelWidth, wallThickness, holeSize)
    ));
    ceilingLeft->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    entities.push_back(std::move(ceilingLeft));
    
    // Right ceiling panel
    auto ceilingRight = std::make_unique<Entity>();
    ceilingRight->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(holeSize/2.0f + panelWidth/2.0f, roomHeight/2.0f, 0.0f), 
        glm::vec3(0.0f), 
        glm::vec3(panelWidth, wallThickness, holeSize)
    ));
    ceilingRight->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    entities.push_back(std::move(ceilingRight));

    // BACK WALL - White, diffuse surface
    auto backWall = std::make_unique<Entity>();
    backWall->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, 0.0f, -roomDepth/2.0f), 
        glm::vec3(0.0f), 
        glm::vec3(roomWidth, roomHeight, wallThickness)
    ));
    backWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // Cornell box white
    entities.push_back(std::move(backWall));

    // LEFT WALL - Red, diffuse surface (Cornell box red)
    auto leftWall = std::make_unique<Entity>();
    leftWall->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(-roomWidth/2.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f), 
        glm::vec3(wallThickness, roomHeight, roomDepth)
    ));
    leftWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.65f, 0.05f, 0.05f))); // Cornell box red
    entities.push_back(std::move(leftWall));

    // RIGHT WALL - Green, diffuse surface (Cornell box green)
    auto rightWall = std::make_unique<Entity>();
    rightWall->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(roomWidth/2.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f), 
        glm::vec3(wallThickness, roomHeight, roomDepth)
    ));
    rightWall->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.12f, 0.45f, 0.15f))); // Cornell box green
    entities.push_back(std::move(rightWall));

    // TALL BOX - Traditional Cornell box placement and proportions
    // Positioned in the back-left area, rotated 18 degrees counterclockwise
    auto tallBox = std::make_unique<Entity>();
    tallBox->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(-1.0f, -0.75f, -1.0f),  // Position: back-left area 
        glm::vec3(0.0f, 18.0f, 0.0f),     // Rotation: 18 degrees around Y axis
        glm::vec3(1.5f, 3.3f, 1.5f)       // Scale: tall rectangular box
    ));
    tallBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(0.73f, 0.73f, 0.73f))); // White like walls
    entities.push_back(std::move(tallBox));

    // SHORT BOX - Traditional Cornell box placement and proportions  
    // Positioned in the front-right area, rotated -18 degrees clockwise
    // EMISSIVE - This box acts as a warm light source
    auto shortBox = std::make_unique<Entity>();
    shortBox->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(1.0f, -1.67f, 0.5f),    // Position: front-right area
        glm::vec3(0.0f, -18.0f, 0.0f),    // Rotation: -18 degrees around Y axis  
        glm::vec3(1.5f, 1.65f, 1.5f)      // Scale: short rectangular box
    ));
    shortBox->addComponent(std::make_unique<MeshComponent>(cubeMesh.get(), glm::vec3(1.0f, 1.0f, 1.0f))); // White base color (will be overridden by material)
    auto emissiveMaterial = MaterialComponent::createEmissive(
        glm::vec3(0.9f, 0.7f, 0.4f),      // Base color: warm orange-white
        glm::vec3(5.0f, 3.5f, 1.5f),      // Emission: much brighter warm light (orange-white, high intensity)
        0.2f,                             // Low roughness (slightly smooth for nice reflections)
        0.0f                              // Non-metallic
    );
    shortBox->addComponent(std::move(emissiveMaterial));
    entities.push_back(std::move(shortBox));

    // AREA LIGHT SOURCE - Positioned above the skylight opening, centered
    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, roomHeight/2.0f + 0.8f, 0.0f)  // Lowered above the ceiling, centered over skylight
    ));
    light->addComponent(std::make_unique<LightComponent>(
        glm::vec3(1.0f, 1.0f, 0.95f),  // Slightly warm white light
        18.0f,                          // Strong intensity to shine down through skylight
        8.0f                            // Large radius for soft area lighting effect
    ));
    entities.push_back(std::move(light));

    // SPINNING TEAPOT - Small teapot in front of the scene
    auto spinningTeapot = std::make_unique<Entity>();
    spinningTeapot->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(-0.5f, -1.8f, 2.0f),    // Position: front-left of the scene
        glm::vec3(0.0f, 0.0f, 0.0f),      // Initial rotation
        glm::vec3(0.4f, 0.4f, 0.4f)       // Small scale - 40% of normal size
    ));
    spinningTeapot->addComponent(std::make_unique<MeshComponent>(teapotMesh.get(), glm::vec3(0.7f, 0.7f, 0.9f))); // Light blue teapot
    spinningTeapot->addComponent(std::make_unique<RotationComponent>(45.0f, glm::vec3(0.0f, 1.0f, 0.0f))); // Rotate around Y-axis at 45 degrees/second
    entities.push_back(std::move(spinningTeapot));

    // Position camera for classic Cornell box viewpoint
    // The camera should be positioned outside the box looking in
    camera.position = glm::vec3(1.0f, 1.8f, 18.5f);  // Much further back to see the entire Cornell box
    camera.yaw = -70.0f;     // Looking toward negative Z (into the box)
    camera.pitch = 0.0f;     // Level with the center of the box
    camera.updateCameraVectors();

    std::cout << "Cornell box scene loaded with " << entities.size() << " entities." << std::endl;
    std::cout << "Enhanced Cornell box configuration:" << std::endl;
    std::cout << "  - White floor, four-panel ceiling with square skylight, and back wall" << std::endl;
    std::cout << "  - Red left wall, green right wall" << std::endl;
    std::cout << "  - Two boxes: tall white box and emissive short box" << std::endl;
    std::cout << "  - Area light source above skylight shining down through opening" << std::endl;
    std::cout << "  - Small spinning teapot in front of scene" << std::endl;
}

/**
 * Scene Selection System Implementation
 */

void Scene::loadScene(SceneType sceneType) {
    currentScene = sceneType;
    
    switch (sceneType) {
        case CORNELL_BOX:
            loadCornellBox();
            break;
        case TEAPOT_LIGHTBOX:
            loadTeapotLightbox();
            break;
        case STONE_FLOOR:
            loadStoneFloorScene();
            break;
        case SHADOW_TEST:
            loadShadowTestScene();
            break;
        case DEFAULT_LIGHTBOX:
            loadDefaultLightbox();
            break;
        case SPONZA_OVERHEAD:
            loadSponzaScene();
            break;
        case GLTF_SPONZA:
            loadGLTFScene("models/Sponza/Sponza.gltf");
            break;
        case ABEAUTIFULGAME:
            loadGLTFScene("models/ABeautifulGame/ABeautifulGame.gltf");
            break;
    }
}

std::string Scene::getSceneName(SceneType sceneType) const {
    switch (sceneType) {
        case CORNELL_BOX: return "Cornell Box";
        case TEAPOT_LIGHTBOX: return "Teapot Lightbox";
        case STONE_FLOOR: return "Stone Floor PBR";
        case SHADOW_TEST: return "Shadow Test";
        case DEFAULT_LIGHTBOX: return "Default Lightbox";
        case SPONZA_OVERHEAD: return "Sponza Overhead";
        case GLTF_SPONZA: return "Sponza (glTF PBR)";
        case ABEAUTIFULGAME: return "A Beautiful Game (glTF)";
        default: return "Unknown Scene";
    }
}

/**
 * Load Sponza Scene with Dramatic Overhead Lighting
 * 
 * Creates a large-scale architectural scene featuring the Sponza atrium model
 * with dramatic overhead lighting to showcase global illumination. This scene
 * demonstrates:
 * - Large architectural spaces
 * - Complex geometry and shadows
 * - Dramatic directional lighting from above
 * - Multi-material surfaces
 * - Realistic scale and proportions
 * 
 * The Sponza model is positioned with strong overhead lighting to create
 * beautiful shadow patterns and demonstrate the GI system's ability to
 * handle complex architectural scenes.
 */
void Scene::loadSponzaScene() {
    // Clear any existing entities
    entities.clear();

    std::cout << "Loading Sponza scene with overhead lighting..." << std::endl;

    // Load the Sponza architectural model
    std::cout << "Attempting to load Sponza model from: models/sponza.obj" << std::endl;
    sponzaMesh = Mesh::loadFromOBJ("models/sponza.obj");
    if (!sponzaMesh) {
        std::cerr << "Failed to load Sponza model, falling back to cube" << std::endl;
        // Fallback to a simple cube if Sponza loading fails
        std::vector<Vertex> fallbackVertices = {
            {glm::vec3(-5.0f, -1.0f, -5.0f),  glm::vec3(0.0f,  1.0f,  0.0f)},
            {glm::vec3(5.0f, -1.0f, -5.0f),   glm::vec3(0.0f,  1.0f,  0.0f)},
            {glm::vec3(5.0f, -1.0f,  5.0f),   glm::vec3(0.0f,  1.0f,  0.0f)},
            {glm::vec3(5.0f, -1.0f,  5.0f),   glm::vec3(0.0f,  1.0f,  0.0f)},
            {glm::vec3(-5.0f, -1.0f,  5.0f),  glm::vec3(0.0f,  1.0f,  0.0f)},
            {glm::vec3(-5.0f, -1.0f, -5.0f),  glm::vec3(0.0f,  1.0f,  0.0f)}
        };
        sponzaMesh = std::make_unique<Mesh>(fallbackVertices);
        std::cout << "Using fallback cube geometry for Sponza scene" << std::endl;
    } else {
        std::cout << "Successfully loaded Sponza model" << std::endl;
    }

    // MAIN SPONZA ARCHITECTURE - Properly scaled two-story building
    auto sponzaBuilding = std::make_unique<Entity>();
    sponzaBuilding->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, 0.0f, 0.0f),      // Position: centered at origin
        glm::vec3(0.0f, 0.0f, 0.0f),      // Rotation: no rotation needed
        glm::vec3(0.1f, 0.1f, 0.1f)       // Scale: 10x larger than tiny, but manageable size
    ));
    sponzaBuilding->addComponent(std::make_unique<MeshComponent>(sponzaMesh.get(), glm::vec3(0.9f, 0.85f, 0.8f))); // Warm stone color
    entities.push_back(std::move(sponzaBuilding));

    // DRAMATIC OVERHEAD LIGHTING SETUP  
    // Main overhead light - positioned above the atrium like sunlight
    auto mainOverheadLight = std::make_unique<Entity>();
    mainOverheadLight->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, 80.0f, 0.0f)     // High above the Sponza atrium (scaled appropriately)
    ));
    mainOverheadLight->addComponent(std::make_unique<LightComponent>(
        glm::vec3(1.0f, 0.95f, 0.85f),   // Warm sunlight color
        250.0f,                          // Strong intensity scaled for 0.1 building
        40.0f                            // Large radius for soft area lighting
    ));
    entities.push_back(std::move(mainOverheadLight));

    // SECONDARY LIGHTING - Fill lights positioned around the atrium
    // Left fill light 
    auto leftFillLight = std::make_unique<Entity>();
    leftFillLight->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(-40.0f, 30.0f, 0.0f)   // Left side, elevated position
    ));
    leftFillLight->addComponent(std::make_unique<LightComponent>(
        glm::vec3(0.8f, 0.9f, 1.0f),     // Cool blue fill light
        80.0f,                           // Scaled intensity 
        25.0f                            // Appropriate radius for lighting
    ));
    entities.push_back(std::move(leftFillLight));

    // Right fill light
    auto rightFillLight = std::make_unique<Entity>();
    rightFillLight->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(40.0f, 30.0f, 0.0f)    // Right side, elevated position
    ));
    rightFillLight->addComponent(std::make_unique<LightComponent>(
        glm::vec3(1.0f, 0.8f, 0.6f),     // Warm orange fill light
        60.0f,                           // Scaled intensity
        25.0f                            // Appropriate radius for lighting
    ));
    entities.push_back(std::move(rightFillLight));

    // ACCENT LIGHTING - Architectural accent lights
    // Front accent light 
    auto frontAccentLight = std::make_unique<Entity>();
    frontAccentLight->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(0.0f, 20.0f, 30.0f)    // Front of atrium, elevated position
    ));
    frontAccentLight->addComponent(std::make_unique<LightComponent>(
        glm::vec3(1.0f, 0.7f, 0.4f),     // Warm accent color
        40.0f,                           // Scaled intensity
        15.0f                            // Medium radius for focused lighting
    ));
    entities.push_back(std::move(frontAccentLight));

    // Position camera to view the Sponza atrium
    // Place camera at a good position to see the architecture 
    camera.position = glm::vec3(0.0f, 5.0f, 25.0f);   // Outside the atrium, elevated view
    camera.yaw = -90.0f;      // Looking toward negative Z (into the atrium)
    camera.pitch = -5.0f;     // Slight downward angle to see the structure
    camera.updateCameraVectors();

    std::cout << "Sponza scene loaded with " << entities.size() << " entities." << std::endl;
    std::cout << "Sponza atrium configuration:" << std::endl;
    std::cout << "  - Sponza atrium at proper architectural scale (10x original)" << std::endl;
    std::cout << "  - Main overhead light positioned like sunlight (250.0 intensity)" << std::endl;
    std::cout << "  - Two elevated fill lights (cool blue left, warm orange right)" << std::endl;
    std::cout << "  - Front accent light for additional illumination" << std::endl;
    std::cout << "  - Camera positioned for optimal architectural viewing" << std::endl;
    std::cout << "  - Perfect for testing GI with complex geometry and dramatic lighting" << std::endl;
} 
/**
 * Load a glTF scene (e.g. glTF Sponza) via the GLTFLoader.
 * One entity per primitive (identity transform - node transforms are baked into the
 * vertices at load), each with its imported PBR material, plus an overhead light scaled
 * to the model bounds. The Scene owns gltfModel, which keeps the meshes/textures alive.
 */
void Scene::loadGLTFScene(const std::string& path) {
    entities.clear();

    gltfModel = loadGLTF(path);
    if (!gltfModel.loaded) {
        std::cerr << "glTF scene not loaded: " << path
                  << "\n  -> Asset missing? Fetch it with: ./scripts/fetch_sponza.sh" << std::endl;
    }

    // Build a renderable entity for every primitive.
    for (auto& inst : gltfModel.instances) {
        auto e = std::make_unique<Entity>();
        e->addComponent(std::make_unique<TransformComponent>(glm::vec3(0.0f))); // baked transform
        e->addComponent(std::make_unique<MeshComponent>(inst.mesh, glm::vec3(1.0f)));
        // Copy the (non-owning) template material so each entity owns its MaterialComponent
        // while still sharing the GLTFModel-owned textures.
        e->addComponent(std::make_unique<MaterialComponent>(std::make_unique<Material>(*inst.material)));
        entities.push_back(std::move(e));
    }

    // Overhead light scaled to the scene extent.
    glm::vec3 center = (gltfModel.boundsMin + gltfModel.boundsMax) * 0.5f;
    glm::vec3 size = gltfModel.boundsMax - gltfModel.boundsMin;
    float extent = std::max(0.001f, glm::length(size));

    auto light = std::make_unique<Entity>();
    light->addComponent(std::make_unique<TransformComponent>(
        glm::vec3(center.x, gltfModel.boundsMax.y * 0.95f, center.z)));
    light->addComponent(std::make_unique<LightComponent>(
        glm::vec3(1.0f, 0.96f, 0.9f), extent * 3.0f, extent * 0.6f));
    entities.push_back(std::move(light));

    // Drop the camera roughly in the middle of the scene looking down its long axis.
    camera.position = glm::vec3(center.x, center.y, center.z);

    std::cout << "glTF scene ready: " << entities.size() << " entities, bounds extent "
              << extent << std::endl;
}
