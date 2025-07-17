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

/**
 * Camera class - First-person 3D camera with smooth controls
 * 
 * Provides a complete camera system for 3D rendering with keyboard movement
 * (WASD) and mouse look controls. Generates view matrices for rendering
 * and maintains proper camera orientation vectors.
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
    // Note: updateCameraVectors is public, so private declaration commented out
    // void updateCameraVectors();
};

#endif // CAMERA_H 