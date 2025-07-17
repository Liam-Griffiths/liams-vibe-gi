/**
 * TransformComponent.h - Spatial Transform Component for Entity-Component-System
 * 
 * Defines the TransformComponent class that handles entity positioning, rotation,
 * and scaling in 3D space. This is a fundamental component that most renderable
 * entities require for proper placement in the world.
 * 
 * Transform Operations:
 * - Position: Translation in world space (x, y, z)
 * - Rotation: Euler angles in degrees (pitch, yaw, roll)
 * - Scale: Non-uniform scaling factors (x, y, z)
 * 
 * Matrix Generation:
 * The component generates model matrices using the standard TRS order:
 * ModelMatrix = Translation * Rotation * Scale
 * 
 * This follows OpenGL conventions and integrates seamlessly with the
 * rendering pipeline for vertex transformation.
 */

#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "Entity.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * TransformComponent - 3D spatial transformation for entities
 * 
 * Manages the position, rotation, and scale of an entity in 3D space.
 * Provides automatic model matrix generation for rendering and physics.
 * Essential component for any entity that needs spatial representation.
 */
class TransformComponent : public Component {
public:
    // Spatial Properties
    glm::vec3 position;         ///< World position (x, y, z) in world units
    glm::vec3 rotation;         ///< Euler angles in degrees (pitch, yaw, roll)
    glm::vec3 scale;            ///< Scale factors (x, y, z) - 1.0 = normal size

    /**
     * Constructor - Initialize transform with default or specified values
     * 
     * @param pos Position vector (default: origin (0,0,0))
     * @param rot Rotation angles in degrees (default: no rotation (0,0,0))
     * @param scl Scale factors (default: normal scale (1,1,1))
     */
    TransformComponent(glm::vec3 pos = glm::vec3(0.0f),
                       glm::vec3 rot = glm::vec3(0.0f),
                       glm::vec3 scl = glm::vec3(1.0f))
        : position(pos), rotation(rot), scale(scl) {}

    /**
     * Generate model matrix for rendering
     * 
     * Computes the 4x4 transformation matrix that converts model-space
     * coordinates to world-space coordinates. Applied in TRS order:
     * 1. Scale the object
     * 2. Rotate around local axes (X, Y, Z order)
     * 3. Translate to world position
     * 
     * @return 4x4 model matrix for vertex shader transformation
     */
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        
        // Apply transformations in TRS order
        model = glm::translate(model, position);                                    // T: Translation
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // R: Rotation X (pitch)
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // R: Rotation Y (yaw)
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // R: Rotation Z (roll)
        model = glm::scale(model, scale);                                           // S: Scale
        
        return model;
    }
};

#endif // TRANSFORMCOMPONENT_H 