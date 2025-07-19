/**
 * RotationComponent.cpp - Implementation of automatic rotation behavior
 */

#define GLM_ENABLE_EXPERIMENTAL
#include "RotationComponent.h"
#include "../include/TransformComponent.h"
#include <glm/gtc/matrix_transform.hpp>

RotationComponent::RotationComponent(float rotationSpeed, glm::vec3 rotationAxis)
    : rotationSpeed(rotationSpeed), currentRotation(0.0f) {
    setRotationAxis(rotationAxis);
}

void RotationComponent::Update(float deltaTime) {
    if (!isEnabled()) return;
    
    auto transform = getTransform();
    if (!transform) return;
    
    // Update the current rotation based on delta time
    currentRotation += rotationSpeed * deltaTime;
    
    // Keep rotation within 0-360 degrees for cleaner values
    if (currentRotation >= 360.0f) {
        currentRotation -= 360.0f;
    } else if (currentRotation < 0.0f) {
        currentRotation += 360.0f;
    }
    
    // Apply rotation to the transform component
    // For Y-axis rotation (most common case), update the Y rotation
    if (rotationAxis.y > 0.9f) {
        transform->rotation.y = currentRotation;
    } else if (rotationAxis.x > 0.9f) {
        transform->rotation.x = currentRotation;
    } else if (rotationAxis.z > 0.9f) {
        transform->rotation.z = currentRotation;
    } else {
        // For arbitrary axes, apply to Y-axis as default
        transform->rotation.y = currentRotation;
    }
}

void RotationComponent::setRotationAxis(const glm::vec3& axis) {
    // Normalize the axis to ensure it's a unit vector
    float length = glm::length(axis);
    if (length > 0.0f) {
        rotationAxis = axis / length;
    } else {
        // Default to Y-axis if invalid axis is provided
        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    }
} 