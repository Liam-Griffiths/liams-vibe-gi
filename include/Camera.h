/**
 * Camera.h - 3D Camera System for Real-time Rendering
 * 
 * Implements a flexible first-person camera system with smooth movement and mouse look.
 * The camera provides view matrix computation for 3D rendering and handles user input
 * for navigation through the 3D scene.
 * 
 * Features:
 * - First-person camera with WASD movement
 * - Mouse look with pitch/yaw rotation
 * - Configurable movement speed and mouse sensitivity
 * - Proper coordinate system handling (right-handed)
 * - Smooth interpolated movement
 * - Frustum culling for performance optimization
 * 
 * Coordinate System:
 * - X: Right direction (positive = right)
 * - Y: Up direction (positive = up)  
 * - Z: Forward direction (positive = towards viewer, negative = into scene)
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

/**
 * Frustum plane representation for culling
 */
struct Plane {
    glm::vec3 normal;  ///< Plane normal vector
    float distance;    ///< Distance from origin

    Plane() = default;
    Plane(const glm::vec3& p1, const glm::vec3& norm) : normal(glm::normalize(norm)), distance(glm::dot(normal, p1)) {}

    /**
     * Calculate signed distance from point to plane
     * Positive = in front of plane, Negative = behind plane
     */
    float getSignedDistanceToPlane(const glm::vec3& point) const {
        return glm::dot(normal, point) - distance;
    }
};

/**
 * Camera frustum for culling calculations
 */
struct Frustum {
    std::array<Plane, 6> planes; ///< Six frustum planes: left, right, bottom, top, near, far
    
    /**
     * Check if a sphere is inside or intersecting the frustum
     * @param center Sphere center position
     * @param radius Sphere radius
     * @return true if sphere is visible (inside or intersecting frustum)
     */
    bool isOnOrForwardPlan(const glm::vec3& center, float radius) const {
        for (const auto& plane : planes) {
            if (plane.getSignedDistanceToPlane(center) < -radius) {
                return false; // Sphere is completely behind this plane
            }
        }
        return true; // Sphere is visible
    }
    
    /**
     * Check if a point is inside the frustum
     * @param point Point to test
     * @return true if point is inside frustum
     */
    bool isPointInFrustum(const glm::vec3& point) const {
        for (const auto& plane : planes) {
            if (plane.getSignedDistanceToPlane(point) < 0) {
                return false;
            }
        }
        return true;
    }
};

/**
 * Camera class - First-person 3D camera with smooth controls and frustum culling
 * 
 * Provides a complete camera system for 3D rendering with keyboard movement
 * (WASD) and mouse look controls. Generates view matrices for rendering
 * and maintains proper camera orientation vectors. Includes frustum culling
 * for performance optimization.
 */
class Camera {
public:
    /**
     * Constructor - Initialize camera with default or specified position
     * 
     * @param position Initial world position of the camera (default: (0,0,3))
     */
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f));

    /**
     * Generate view matrix for rendering
     * The view matrix transforms world coordinates to camera/view space
     * 
     * @return 4x4 view matrix for use in vertex shaders
     */
    glm::mat4 getViewMatrix() const;
    
    /**
     * Update camera frustum for culling calculations
     * Call this whenever the camera or projection matrix changes
     * 
     * @param projectionMatrix Current projection matrix
     */
    void updateFrustum(const glm::mat4& projectionMatrix);
    
    /**
     * Check if a sphere (representing an object's bounding sphere) is visible
     * @param center Sphere center in world space
     * @param radius Sphere radius
     * @return true if sphere is visible (should be rendered)
     */
    bool isSphereInFrustum(const glm::vec3& center, float radius) const;
    
    /**
     * Check if a point is visible in the camera frustum
     * @param point Point in world space
     * @return true if point is visible
     */
    bool isPointInFrustum(const glm::vec3& point) const;
    
    /**
     * Get the current camera frustum
     * @return Reference to the current frustum
     */
    const Frustum& getFrustum() const { return currentFrustum; }
    
    /**
     * Process keyboard input for camera movement
     * Handles WASD movement in the camera's local coordinate system
     * 
     * @param direction Movement direction (0=forward, 1=backward, 2=left, 3=right)
     * @param deltaTime Frame time delta for frame-rate independent movement
     */
    void processKeyboard(int direction, float deltaTime);
    
    /**
     * Process mouse input for camera rotation (look around)
     * Updates camera orientation based on mouse movement
     * 
     * @param xoffset Mouse movement in X direction (horizontal)
     * @param yoffset Mouse movement in Y direction (vertical)
     */
    void processMouse(float xoffset, float yoffset);

    // Camera Position and Orientation
    glm::vec3 position;         ///< Camera world position
    glm::vec3 front;            ///< Forward direction vector (where camera is looking)
    glm::vec3 up;               ///< Up direction vector (camera's local up)
    glm::vec3 right;            ///< Right direction vector (camera's local right)
    glm::vec3 worldUp;          ///< World up vector (typically (0,1,0))

    // Camera Rotation (Euler angles in degrees)
    float yaw;                  ///< Horizontal rotation (left/right) in degrees
    float pitch;                ///< Vertical rotation (up/down) in degrees

    // Camera Control Parameters
    float movementSpeed;        ///< Movement speed in units per second
    float mouseSensitivity;     ///< Mouse sensitivity multiplier

    /**
     * Update camera direction vectors based on current yaw and pitch
     * Recalculates front, right, and up vectors from Euler angles
     * Called automatically when orientation changes
     */
    void updateCameraVectors();

private:
    Frustum currentFrustum;     ///< Current camera frustum for culling
    
    /**
     * Create a frustum plane from three points
     * @param a First point
     * @param b Second point  
     * @param c Third point
     * @return Plane created from the three points
     */
    Plane createPlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const;
};

#endif // CAMERA_H 