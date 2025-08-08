
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
    // Correct frustum plane extraction using column vectors (GLM is column-major)
    const glm::mat4 viewProjection = projectionMatrix * getViewMatrix();

    const glm::vec4 c0 = viewProjection[0]; // column 0
    const glm::vec4 c1 = viewProjection[1]; // column 1
    const glm::vec4 c2 = viewProjection[2]; // column 2
    const glm::vec4 c3 = viewProjection[3]; // column 3

    auto setPlane = [&](int idx, const glm::vec4& p) {
        glm::vec3 n(p.x, p.y, p.z);
        float d = p.w; // plane equation: dot(n,x) + d = 0
        float invLen = 1.0f / glm::length(n);
        currentFrustum.planes[idx].normal = n * invLen;
        // Our Plane uses dot(normal, point) - distance >= 0, so store distance = -d
        currentFrustum.planes[idx].distance = (-d) * invLen;
    };

    // Left, Right, Bottom, Top, Near, Far
    setPlane(0, c3 + c0);
    setPlane(1, c3 - c0);
    setPlane(2, c3 + c1);
    setPlane(3, c3 - c1);
    setPlane(4, c3 + c2);
    setPlane(5, c3 - c2);
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