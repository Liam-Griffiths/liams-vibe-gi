/**
 * RotationComponent.h - Component for automatic rotation behavior
 * 
 * This component provides automatic rotation functionality for entities.
 * When attached to an entity with a TransformComponent, it will continuously
 * rotate the object around the specified axis at a given speed.
 */

#ifndef ROTATION_COMPONENT_H
#define ROTATION_COMPONENT_H

#include "Behaviour.h"
#include <memory>
#include <glm/glm.hpp>

/**
 * RotationComponent - Provides automatic rotation behavior
 * 
 * This component can be attached to any entity to make it rotate
 * continuously around a specified axis. The rotation is frame-rate
 * independent and uses delta time for smooth animation.
 */
class RotationComponent : public Behaviour {
public:
    /**
     * Constructor
     * @param rotationSpeed Speed of rotation in degrees per second
     * @param rotationAxis Axis around which to rotate (default: Y-axis)
     */
    RotationComponent(float rotationSpeed = 30.0f, glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f));

    /**
     * Called every frame to update rotation
     * @param deltaTime Time elapsed since last frame in seconds
     */
    void Update(float deltaTime) override;

    /**
     * Get the current rotation speed
     * @return Rotation speed in degrees per second
     */
    float getRotationSpeed() const { return rotationSpeed; }

    /**
     * Set the rotation speed
     * @param speed New rotation speed in degrees per second
     */
    void setRotationSpeed(float speed) { rotationSpeed = speed; }

    /**
     * Get the rotation axis
     * @return Current rotation axis as a normalized vector
     */
    glm::vec3 getRotationAxis() const { return rotationAxis; }

    /**
     * Set the rotation axis
     * @param axis New rotation axis (will be normalized)
     */
    void setRotationAxis(const glm::vec3& axis);

    /**
     * Get the current accumulated rotation in degrees
     * @return Current rotation angle in degrees
     */
    float getCurrentRotation() const { return currentRotation; }

private:
    float rotationSpeed;        ///< Rotation speed in degrees per second
    glm::vec3 rotationAxis;     ///< Axis around which to rotate (normalized)
    float currentRotation;      ///< Current accumulated rotation in degrees
};

#endif // ROTATION_COMPONENT_H 