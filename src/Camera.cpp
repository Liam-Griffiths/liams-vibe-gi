
// Camera.cpp
#include "../include/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 pos) : position(pos),
    front(glm::vec3(0.0f, 0.0f, -1.0f)),
    up(glm::vec3(0.0f, 1.0f, 0.0f)),
    right(glm::vec3(1.0f, 0.0f, 0.0f)),
    worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    yaw(-90.0f),
    pitch(0.0f),
    movementSpeed(2.5f),
    mouseSensitivity(0.1f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::updateFrustum(const glm::mat4& projectionMatrix) {
    // Calculate view-projection matrix
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 viewProjection = projectionMatrix * viewMatrix;
    
    // Extract frustum planes from view-projection matrix
    // The frustum planes are extracted from the combined view-projection matrix
    // using the method described in "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix"
    
    // Left plane
    currentFrustum.planes[0].normal.x = viewProjection[0][3] + viewProjection[0][0];
    currentFrustum.planes[0].normal.y = viewProjection[1][3] + viewProjection[1][0];
    currentFrustum.planes[0].normal.z = viewProjection[2][3] + viewProjection[2][0];
    currentFrustum.planes[0].distance = viewProjection[3][3] + viewProjection[3][0];
    
    // Right plane
    currentFrustum.planes[1].normal.x = viewProjection[0][3] - viewProjection[0][0];
    currentFrustum.planes[1].normal.y = viewProjection[1][3] - viewProjection[1][0];
    currentFrustum.planes[1].normal.z = viewProjection[2][3] - viewProjection[2][0];
    currentFrustum.planes[1].distance = viewProjection[3][3] - viewProjection[3][0];
    
    // Bottom plane
    currentFrustum.planes[2].normal.x = viewProjection[0][3] + viewProjection[0][1];
    currentFrustum.planes[2].normal.y = viewProjection[1][3] + viewProjection[1][1];
    currentFrustum.planes[2].normal.z = viewProjection[2][3] + viewProjection[2][1];
    currentFrustum.planes[2].distance = viewProjection[3][3] + viewProjection[3][1];
    
    // Top plane
    currentFrustum.planes[3].normal.x = viewProjection[0][3] - viewProjection[0][1];
    currentFrustum.planes[3].normal.y = viewProjection[1][3] - viewProjection[1][1];
    currentFrustum.planes[3].normal.z = viewProjection[2][3] - viewProjection[2][1];
    currentFrustum.planes[3].distance = viewProjection[3][3] - viewProjection[3][1];
    
    // Near plane
    currentFrustum.planes[4].normal.x = viewProjection[0][3] + viewProjection[0][2];
    currentFrustum.planes[4].normal.y = viewProjection[1][3] + viewProjection[1][2];
    currentFrustum.planes[4].normal.z = viewProjection[2][3] + viewProjection[2][2];
    currentFrustum.planes[4].distance = viewProjection[3][3] + viewProjection[3][2];
    
    // Far plane
    currentFrustum.planes[5].normal.x = viewProjection[0][3] - viewProjection[0][2];
    currentFrustum.planes[5].normal.y = viewProjection[1][3] - viewProjection[1][2];
    currentFrustum.planes[5].normal.z = viewProjection[2][3] - viewProjection[2][2];
    currentFrustum.planes[5].distance = viewProjection[3][3] - viewProjection[3][2];
    
    // Normalize all planes
    for (auto& plane : currentFrustum.planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }
}

bool Camera::isSphereInFrustum(const glm::vec3& center, float radius) const {
    return currentFrustum.isOnOrForwardPlan(center, radius);
}

bool Camera::isPointInFrustum(const glm::vec3& point) const {
    return currentFrustum.isPointInFrustum(point);
}

Plane Camera::createPlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const {
    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    return Plane(a, normal);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == 0) // forward
        position += front * velocity;
    if (direction == 1) // backward
        position -= front * velocity;
    if (direction == 2) // left
        position -= right * velocity;
    if (direction == 3) // right
        position += right * velocity;
}

void Camera::processMouse(float xoffset, float yoffset) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
} 