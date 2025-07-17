/**
 * Scene.h - Scene Management and Entity-Component-System
 * 
 * This file defines the Scene class which manages all entities in the world
 * using an Entity-Component-System (ECS) architecture. The scene handles:
 * 
 * - Entity lifecycle management
 * - Camera management
 * - Mesh storage and reuse
 * - Scene loading and configuration
 * 
 * The ECS pattern allows for flexible composition of game objects by
 * combining different components (Transform, Mesh, Material, Light, etc.)
 * onto entities, providing better modularity than traditional inheritance.
 */

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>

#include "Entity.h"
#include "Camera.h"
#include "Mesh.h"

/**
 * Scene class - Central manager for all world entities and rendering data
 * 
 * The Scene class implements the core ECS (Entity-Component-System) pattern
 * and manages all objects in the 3D world. It provides scene loading functionality
 * for different demo scenarios and maintains shared resources like meshes.
 */
class Scene {
public:
    // ECS Management
    std::vector<std::unique_ptr<Entity>> entities;  ///< All entities in the scene (ECS pattern)
    
    // Camera System
    Camera camera;                                   ///< Main scene camera for rendering
    
    // Shared Mesh Resources (for performance - avoid duplicate loading)
    std::unique_ptr<Mesh> cubeMesh;                 ///< Shared cube geometry
    std::unique_ptr<Mesh> teapotMesh;               ///< Shared teapot model geometry
    std::unique_ptr<Mesh> floorMesh;                ///< Shared floor plane geometry

    /**
     * Constructor - Initializes scene and loads default content
     */
    Scene();
    
    /**
     * Scene Loading Functions
     * These methods set up different demo scenarios with appropriate
     * lighting, geometry, and materials for testing various rendering features.
     */
    
    /**
     * Load a basic lightbox scene with simple geometry
     * Good for testing basic lighting and shadows
     */
    void loadDefaultLightbox();
    
    /**
     * Load a scene specifically designed for shadow testing
     * Includes multiple objects at various distances from light
     */
    void loadShadowTestScene();
    
    /**
     * Load the main teapot lightbox demo scene
     * Features the Utah teapot with PBR materials in a controlled lighting environment
     */
    void loadTeapotLightbox();
    
    /**
     * Load a scene with stone floor materials
     * Demonstrates PBR material system with detailed textures
     */
    void loadStoneFloorScene();
};

#endif 