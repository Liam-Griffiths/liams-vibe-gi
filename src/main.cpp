/**
 * main.cpp - Vibe-GI Renderer Entry Point
 * 
 * This is the main entry point for the vibe-gi renderer, implementing a sophisticated
 * real-time global illumination system using radiance cascades. The renderer features:
 * 
 * - Radiance Cascades GI: Multi-scale indirect lighting computation
 * - Deferred Rendering: G-buffer based lighting pipeline
 * - Temporal Anti-Aliasing (TAA): Motion-based temporal upsampling
 * - Screen Space Ambient Occlusion (SSAO): Real-time ambient occlusion
 * - PBR Materials: Physically based material system
 * - Shadow Mapping: Directional light shadow casting
 * 
 * The rendering pipeline follows these main passes:
 * 1. Shadow Map Generation
 * 2. G-Buffer Pass (geometry data)
 * 3. SSAO Computation
 * 4. Radiance Cascades GI
 * 5. Final Composite
 * 6. Temporal Anti-Aliasing
 * 
 * Controls:
 * - WASD: Camera movement
 * - Mouse: Look around
 * - Arrow Keys: Move light
 * - K/L: Light height
 * - O/P: Light intensity
 * - I/U: Light radius
 * - M: Toggle ambient lighting
 * - G: Toggle global illumination
 * - T: Toggle SSAO
 * - R: Reset temporal accumulation
 * - Space: Pause/unpause
 * - ESC: Exit
 */

#include <iostream>
#include <vector>
#include <memory>

// Core rendering components
#include "../include/Window.h"
#include "../include/Camera.h"
#include "../include/Shader.h"
#include "../include/Mesh.h"

// Entity-Component-System architecture
#include "../include/Entity.h"
#include "../include/TransformComponent.h"
#include "../include/MeshComponent.h"
#include "../include/MaterialComponent.h"
#include "../include/LightComponent.h"
#include "../include/Scene.h"

// Advanced rendering features
#include "../include/ShadowMap.h"
#include "../include/FullscreenQuad.h"
#include "../include/TextRenderer.h"
#include "../include/RadianceCascades.h"

#include <string>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>
#include <algorithm>

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, Camera& camera, Scene& scene, RadianceCascades& rc, float deltaTime, bool& ambientEnabled, bool& giEnabled, bool& ssaoEnabled, bool& paused, float& pausedTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Global variables for mouse input handling
bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0f / 2.0;

/**
 * Main rendering loop and application entry point
 * 
 * Initializes the rendering system, sets up the complete graphics pipeline,
 * and runs the main game loop with real-time global illumination.
 */
int main() {
    try {
        // Initialize main window with OpenGL context
        Window window(800, 600, "Vibe-GI: Global Illumination Renderer");

        // Set up window callbacks for input handling
        glfwSetFramebufferSizeCallback(window.getGLFWWindow(), framebuffer_size_callback);
        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window.getGLFWWindow(), mouse_callback);

        // Enable depth testing for proper 3D rendering
        glEnable(GL_DEPTH_TEST);

        // Initialize all shaders for the rendering pipeline
        Shader lightingShader("shaders/lighting.vert", "shaders/lighting.frag");           // Forward lighting (legacy)
        Shader shadowShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");     // Shadow map generation
        Shader gBufferShader("shaders/gbuffer.vert", "shaders/gbuffer.frag");             // Deferred geometry pass
        Shader rcShader("shaders/fullscreen.vert", "shaders/rc_cascade.frag");            // Radiance cascades computation
        Shader blurShader("shaders/fullscreen.vert", "shaders/blur.frag");                // GI temporal blur
        Shader compositeShader("shaders/fullscreen.vert", "shaders/final_composite.frag"); // Final lighting composite
        Shader taaShader("shaders/fullscreen.vert", "shaders/taa.frag");                  // Temporal anti-aliasing
        Shader ssaoShader("shaders/fullscreen.vert", "shaders/ssao.frag");                // Screen-space ambient occlusion
        Shader ssaoBlurShader("shaders/fullscreen.vert", "shaders/ssao_blur.frag");       // SSAO blur for noise reduction
        Shader textShader("shaders/text.vert", "shaders/text.frag");                      // UI text rendering
        
        // Initialize text rendering system for debug UI
        TextRenderer textRenderer("fonts/OpenSans-Regular.ttf", 24, &textShader);

        // Initialize core rendering systems
        ShadowMap shadowMap;                                    // Directional light shadow mapping
        RadianceCascades rc(800, 600, 6);                      // 6-cascade radiance cascade GI system
        FullscreenQuad quad;                                    // Fullscreen quad for post-processing

        // Create offscreen framebuffer for composite pass (before TAA)
        // This allows us to apply temporal anti-aliasing as a final step
        unsigned int compositeFBO;
        unsigned int compositeTexture;
        glGenFramebuffers(1, &compositeFBO);
        glGenTextures(1, &compositeTexture);
        glBindTexture(GL_TEXTURE_2D, compositeTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 600, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositeTexture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Composite FBO incomplete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create scene with ECS architecture
        Scene scene;

        // Set up camera for mouse input
        glfwSetWindowUserPointer(window.getGLFWWindow(), &scene.camera);

        // Timing and performance tracking variables
        bool ambientEnabled = true;     // Toggle for ambient lighting
        bool giEnabled = true;          // Toggle for global illumination
        bool ssaoEnabled = true;        // Toggle for screen space ambient occlusion
        float deltaTime = 0.0f;         // Frame time delta
        float lastFrame = 0.0f;         // Previous frame timestamp
        int frameCount = 0;             // Frame counter for FPS calculation
        float fpsTimer = 0.0f;          // FPS calculation timer
        int fps = 0;                    // Current FPS
        bool paused = false;            // Pause state
        float pausedTime = 0.0f;        // Time when paused
        static int lastWidth = 0;       // Previous frame width (for resize detection)
        static int lastHeight = 0;      // Previous frame height (for resize detection)

        /**
         * MAIN RENDER LOOP
         * 
         * This loop implements the complete real-time rendering pipeline with
         * global illumination, running at interactive frame rates.
         */
        while (!window.shouldClose()) {
            // Calculate frame timing for smooth animation and movement
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // Update FPS counter
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                fps = static_cast<int>(frameCount / fpsTimer);
                frameCount = 0;
                fpsTimer -= 1.0f;
            }

            // Process user input (camera movement, light controls, toggles)
            processInput(window.getGLFWWindow(), scene.camera, scene, rc, deltaTime, ambientEnabled, giEnabled, ssaoEnabled, paused, pausedTime);

            // Extract light information from ECS for rendering
            // In a real engine, this would support multiple lights
            glm::vec3 lightPos(0.0f);
            glm::vec3 lightColor(1.0f);
            float lightRadius = 2.0f; // Default radius for light attenuation
            static glm::vec3 lastLightPos(0.0f);
            
            // Find the primary light in the scene
            for (const auto& entity : scene.entities) {
                if (auto light = entity->getComponent<LightComponent>()) {
                    if (auto transform = entity->getComponent<TransformComponent>()) {
                        lightPos = transform->position;
                        lightColor = light->color * light->intensity;
                        lightRadius = light->radius;
                    }
                }
            }
            
            // Reset temporal accumulation if light moved significantly
            // This prevents ghosting artifacts when lighting changes rapidly
            float lightMovement = glm::length(lightPos - lastLightPos);
            if (lightMovement > 0.1f) { // Threshold for significant movement
                rc.resetTemporalAccumulation();
                lastLightPos = lightPos;
            }

            // Calculate light space matrix for shadow mapping
            // This defines the light's view for shadow map generation
            glm::mat4 lightSpaceMatrix = shadowMap.getLightSpaceMatrix(lightPos, lightRadius);

            // Handle window resizing - only update resources when size actually changes
            int width, height;
            glfwGetFramebufferSize(window.getGLFWWindow(), &width, &height);
            if (width != lastWidth || height != lastHeight) {
                rc.resize(width, height);
                lastWidth = width;
                lastHeight = height;
                glViewport(0, 0, width, height);
            }

            // Update camera matrices with correct aspect ratio
            float aspectRatio = (float)width / (float)height;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
            glm::mat4 view = scene.camera.getViewMatrix();

            // Store previous frame matrices for temporal effects (TAA, motion vectors)
            static glm::mat4 previousView = view;
            static glm::mat4 previousProjection = projection;

            // Poll window events (input, resize, etc.)
            window.pollEvents();

            /**
             * RENDERING PIPELINE - Multi-pass deferred rendering with global illumination
             */

            // PASS 1: SHADOW MAP GENERATION
            // Render scene from light's perspective to generate shadow map
            shadowShader.use();
            shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shadowMap.bindForWriting();
            for (const auto& entity : scene.entities) {
                auto meshComp = entity->getComponent<MeshComponent>();
                if (meshComp) {
                    auto transform = entity->getComponent<TransformComponent>();
                    if (transform) {
                        shadowShader.setMat4("model", transform->getModelMatrix());
                        meshComp->mesh->Draw(shadowShader.ID);
                    }
                }
            }

            // PASS 2: G-BUFFER GENERATION (Deferred Rendering)
            // Render geometry data (position, normal, albedo, motion vectors) to textures
            rc.bindGBufferForWriting();
            gBufferShader.use();
            gBufferShader.setMat4("projection", projection);
            gBufferShader.setMat4("view", view);
            gBufferShader.setMat4("previousProjection", previousProjection);
            gBufferShader.setMat4("previousView", previousView);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Render all scene geometry to G-buffer
            for (const auto& entity : scene.entities) {
                auto meshComp = entity->getComponent<MeshComponent>();
                auto transformComp = entity->getComponent<TransformComponent>();
                auto materialComp = entity->getComponent<MaterialComponent>();

                if (meshComp && transformComp && meshComp->mesh) {
                    gBufferShader.setMat4("model", transformComp->getModelMatrix());
                    gBufferShader.setVec3("objectColor", meshComp->color);
                    
                    // Apply PBR material properties if available
                    if (materialComp && materialComp->material) {
                        materialComp->material->setUniforms(gBufferShader.ID);
                        materialComp->material->bindTextures();
                    } else {
                        // Set default material parameters when no material is present
                        gBufferShader.setBool("hasMaterial", false);
                    }
                    
                    meshComp->mesh->Draw(gBufferShader.ID);
                    
                    // Clean up texture bindings
                    if (materialComp && materialComp->material) {
                        materialComp->material->unbindTextures();
                    }
                }
            }
            
            // PASS 3: SCREEN SPACE AMBIENT OCCLUSION (SSAO)
            // Compute ambient occlusion for enhanced depth perception (if enabled)
            if (ssaoEnabled) {
                rc.computeSSAO(ssaoShader, projection);
                
                // PASS 4: SSAO BLUR
                // Smooth the SSAO to reduce noise while preserving detail
                rc.blurSSAO(ssaoBlurShader);
            }
            
            // PASS 5: RADIANCE CASCADES GLOBAL ILLUMINATION
            // Compute multi-bounce indirect lighting using radiance cascades
            if (giEnabled) {
                rcShader.use();
                rcShader.setMat4("invView", glm::inverse(view)); // For world space calculations
                rcShader.setVec3("lightPos", lightPos);          // World space light position
                rcShader.setVec3("lightColor", lightColor);      // Light color and intensity
                rcShader.setFloat("lightRadius", lightRadius);   // Light attenuation radius
                rcShader.setFloat("time", glfwGetTime());         // Time for temporal effects
                rc.compute(rcShader, view, projection);
                
                // PASS 6: GI TEMPORAL BLUR
                // Apply blur to GI for temporal stability and noise reduction
                rc.blur(blurShader);
            }
            
            // PASS 7: FINAL COMPOSITE TO OFFSCREEN BUFFER
            // Combine all lighting contributions into final image
            glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
            glViewport(0, 0, width, height);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            
            compositeShader.use();
            compositeShader.setMat4("view", view);
            compositeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            compositeShader.setVec3("lightPos", lightPos);
            compositeShader.setVec3("lightColor", lightColor);
            compositeShader.setVec3("viewPos", scene.camera.position);
            compositeShader.setFloat("lightRadius", lightRadius);
            compositeShader.setFloat("ssgiStrength", giEnabled ? 2.0f : 0.0f); // Conditional GI strength
            compositeShader.setFloat("ambientStrength", ambientEnabled ? 0.15f : 0.0f);
            compositeShader.setFloat("ssaoStrength", ssaoEnabled ? 1.0f : 0.0f); // Conditional SSAO contribution
            
            // Bind all G-buffer textures for lighting calculations
            compositeShader.setInt("gPosition", 0);
            compositeShader.setInt("gNormal", 1);
            compositeShader.setInt("gAlbedo", 2);
            compositeShader.setInt("shadowMap", 3);
            compositeShader.setInt("ssaoTexture", 10);
            
            // Bind radiance cascade textures (multi-scale GI data)
            for (int i = 0; i < 6; ++i) {
                compositeShader.setInt("rcTexture[" + std::to_string(i) + "]", 4 + i);
            }
            
            // Activate and bind all required textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rc.getGPosition());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, rc.getGNormal());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, rc.getGAlbedo());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, shadowMap.depthMap);
            
            // Bind SSAO texture
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, rc.getSSAOBlurTexture());
            
            // Bind all cascade textures for GI sampling
            for (int i = 0; i < 6; ++i) {
                glActiveTexture(GL_TEXTURE4 + i);
                glBindTexture(GL_TEXTURE_2D, rc.getTexture(i));
            }

            // Render fullscreen quad to perform lighting calculations
            quad.render();
            
            glEnable(GL_DEPTH_TEST);

            // PASS 8: TEMPORAL ANTI-ALIASING (TAA) TO SCREEN
            // Apply temporal upsampling to reduce aliasing artifacts
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            taaShader.use();
            taaShader.setInt("currentFrame", 0);
            taaShader.setInt("historyFrame", 1);
            taaShader.setInt("velocityBuffer", 2);
            taaShader.setInt("depthBuffer", 3);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, compositeTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, rc.getHistoryTexture());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, rc.getGVelocity());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, rc.getGPosition()); // Using position.z as depth

            quad.render();

            glEnable(GL_DEPTH_TEST);

            // Update temporal history for next frame
            // Copy current composite result to history texture for TAA
            glBindFramebuffer(GL_READ_FRAMEBUFFER, compositeFBO);
            glBindTexture(GL_TEXTURE_2D, rc.getHistoryTexture());
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);

            // PASS 9: DEBUG UI RENDERING
            // Render performance metrics and control information
            if (true) { // Debug info always enabled for now
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_DEPTH_TEST);
                glm::mat4 ortho = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
                
                // Render UI text (positioned at bottom left per user preference [[memory:3304406]])
                textRenderer.RenderText("Vibe-GI: Real-time Global Illumination", 25.0f, 200.0f, 0.7f, glm::vec3(0.0, 1.0, 0.0), ortho);
                textRenderer.RenderText("WASD: Move camera", 25.0f, 170.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                textRenderer.RenderText("Mouse: Look around", 25.0f, 150.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                textRenderer.RenderText("M: Toggle Ambient, G: Toggle GI, T: Toggle SSAO", 25.0f, 130.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                textRenderer.RenderText("Arrow Keys: Move Light, K/L: Height", 25.0f, 110.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                textRenderer.RenderText("O/P: Light Intensity, I/U: Light Radius", 25.0f, 90.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                std::string fpsText = "FPS: " + std::to_string(fps);
                textRenderer.RenderText(fpsText, 25.0f, 70.0f, 0.5f, glm::vec3(1.0, 1.0, 1.0), ortho);
                std::string giStatusText = "GI: " + std::string(giEnabled ? "ON" : "OFF");
                textRenderer.RenderText(giStatusText, 25.0f, 50.0f, 0.5f, giEnabled ? glm::vec3(0.0, 1.0, 0.0) : glm::vec3(1.0, 0.0, 0.0), ortho);
                std::string ssaoStatusText = "SSAO: " + std::string(ssaoEnabled ? "ON" : "OFF");
                textRenderer.RenderText(ssaoStatusText, 25.0f, 30.0f, 0.5f, ssaoEnabled ? glm::vec3(0.0, 1.0, 0.0) : glm::vec3(1.0, 0.0, 0.0), ortho);
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
            }

            // Store matrices for next frame's temporal effects
            previousView = view;
            previousProjection = projection;

            // Present final frame to screen
            window.swapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

/**
 * Callback function for window resize events
 * Updates the OpenGL viewport when window is resized
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

/**
 * Main input processing function
 * 
 * Handles all keyboard input for camera movement, light control,
 * and rendering option toggles. Includes pause functionality.
 * 
 * @param window        GLFW window handle
 * @param camera        Camera object for movement
 * @param scene         Scene containing entities
 * @param rc            Radiance cascades system
 * @param deltaTime     Frame time delta for smooth movement
 * @param ambientEnabled Reference to ambient lighting toggle
 * @param giEnabled     Reference to GI toggle
 * @param ssaoEnabled   Reference to SSAO toggle
 * @param paused        Reference to pause state
 * @param pausedTime    Time when pause was activated
 */
void processInput(GLFWwindow *window, Camera& camera, Scene& scene, RadianceCascades& rc, float deltaTime, bool& ambientEnabled, bool& giEnabled, bool& ssaoEnabled, bool& paused, float& pausedTime) {
    // Toggle ambient lighting with M key
    static bool lastM = false;
    bool currentM = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (!lastM && currentM) {
        ambientEnabled = !ambientEnabled;
    }
    lastM = currentM;
    
    // Toggle global illumination with G key
    static bool lastG = false;
    bool currentG = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    if (!lastG && currentG) {
        giEnabled = !giEnabled;
    }
    lastG = currentG;
    
    // Toggle SSAO with T key
    static bool lastT = false;
    bool currentT = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    if (!lastT && currentT) {
        ssaoEnabled = !ssaoEnabled;
    }
    lastT = currentT;
    
    // Reset temporal accumulation with R key (useful when lighting changes)
    static bool lastR = false;
    bool currentR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (!lastR && currentR) {
        rc.resetTemporalAccumulation();
    }
    lastR = currentR;
    
    // Pause/unpause with Space key
    static bool lastPause = false;
    bool currentPause = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (!lastPause && currentPause) {
        paused = !paused;
        if (paused) {
            pausedTime = glfwGetTime();
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    lastPause = currentPause;
    
    // Only process movement input when not paused
    if (!paused) {
        // Exit application
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
            
        // Camera movement (WASD)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(0, deltaTime);  // Forward
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(1, deltaTime);  // Backward
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(2, deltaTime);  // Left
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(3, deltaTime);  // Right
            
        // Light movement controls
        float lightSpeed = 3.0f * deltaTime;
        
        // Find the light entity and update its position
        for (const auto& entity : scene.entities) {
            if (auto light = entity->getComponent<LightComponent>()) {
                if (auto transform = entity->getComponent<TransformComponent>()) {
                    // Arrow keys for X/Z movement (horizontal plane)
                    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                        transform->position.x -= lightSpeed;
                    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                        transform->position.x += lightSpeed;
                    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                        transform->position.z -= lightSpeed;
                    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                        transform->position.z += lightSpeed;
                        
                    // K/L keys for Y movement (vertical)
                    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
                        transform->position.y += lightSpeed;
                    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
                        transform->position.y -= lightSpeed;
                        
                    // O/P keys for light intensity adjustment
                    float intensitySpeed = 1.0f * deltaTime;
                    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
                        light->intensity += intensitySpeed;
                    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
                        light->intensity = std::max(0.0f, light->intensity - intensitySpeed);
                        
                    // I/U keys for light radius (size/attenuation)
                    float radiusSpeed = 1.0f * deltaTime;
                    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
                        light->radius += radiusSpeed;
                    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
                        light->radius = std::max(0.5f, light->radius - radiusSpeed);
                        
                    break; // Only move the first light found
                }
            }
        }
    }
}

/**
 * Mouse callback function for camera look controls
 * 
 * Processes mouse movement to control camera orientation.
 * Handles first-mouse detection to prevent camera jumping.
 */
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    static bool firstMouse = true;
    static float lastX = 800.0f / 2.0f;
    static float lastY = 600.0f / 2.0f;

    // Prevent camera jump on first mouse input
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Calculate mouse movement delta
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed Y for proper camera movement

    lastX = xpos;
    lastY = ypos;

    // Apply mouse movement to camera
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->processMouse(xoffset, yoffset);
    }
}
