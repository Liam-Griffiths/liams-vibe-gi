// main.cpp
#include <iostream>
#include <vector>
#include <memory>

#include "../include/Window.h"
#include "../include/Camera.h"
#include "../include/Shader.h"
#include "../include/Mesh.h"
#include "../include/Entity.h"
#include "../include/TransformComponent.h"
#include "../include/MeshComponent.h"
#include "../include/LightComponent.h"
#include "../include/Scene.h"
#include "../include/ShadowMap.h"
#include "../include/SSGI.h"
#include "../include/FullscreenQuad.h"
#include <string>
#include "../include/TextRenderer.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, Camera& camera, Scene& scene, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Global variables for mouse input
bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0f / 2.0;

// ECS: List of entities
// std::vector<std::unique_ptr<Entity>> entities;

int main() {
    try {
        Window window(800, 600, "Basic 3D Engine");

        // Set callbacks
        glfwSetFramebufferSizeCallback(window.getGLFWWindow(), framebuffer_size_callback);
        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window.getGLFWWindow(), mouse_callback);

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);

        // Create shaders
        Shader lightingShader("../shaders/lighting.vert", "../shaders/lighting.frag");
        Shader shadowShader("../shaders/shadow_depth.vert", "../shaders/shadow_depth.frag");
        Shader gBufferShader("../shaders/gbuffer.vert", "../shaders/gbuffer.frag");
        Shader ssgiShader("../shaders/ssgi.vert", "../shaders/ssgi.frag");
        Shader ssgiBlurShader("../shaders/ssgi.vert", "../shaders/ssgi_blur.frag");
        Shader compositeShader("../shaders/ssgi.vert", "../shaders/final_composite.frag");
        Shader textShader("../shaders/text.vert", "../shaders/text.frag");
        TextRenderer textRenderer("fonts/OpenSans-Regular.ttf", 24, &textShader);

        // Create shadow map and SSGI system
        ShadowMap shadowMap;
        SSGI ssgi(800, 600);
        FullscreenQuad quad;

        // Create scene
        Scene scene;

        // Camera setup
        glfwSetWindowUserPointer(window.getGLFWWindow(), &scene.camera);

        // Timing
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        int frameCount = 0;
        float fpsTimer = 0.0f;
        int fps = 0;

        // Projection matrix (will be updated each frame with correct aspect ratio)

        while (!window.shouldClose()) {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                fps = static_cast<int>(frameCount / fpsTimer);
                frameCount = 0;
                fpsTimer -= 1.0f;
            }

            processInput(window.getGLFWWindow(), scene.camera, scene, deltaTime);

            // Find light position and color
            glm::vec3 lightPos(0.0f);
            glm::vec3 lightColor(1.0f);
            for (const auto& entity : scene.entities) {
                if (auto light = entity->getComponent<LightComponent>()) {
                    if (auto transform = entity->getComponent<TransformComponent>()) {
                        lightPos = transform->position;
                        lightColor = light->color * light->intensity;
                    }
                }
            }

            // Calculate light space matrix
            glm::mat4 lightSpaceMatrix = shadowMap.getLightSpaceMatrix(lightPos);

            // Get current framebuffer size and update SSGI if needed
            int width, height;
            glfwGetFramebufferSize(window.getGLFWWindow(), &width, &height);
            ssgi.resizeBuffers(width, height);
            
            // Update projection matrix with correct aspect ratio
            float aspectRatio = (float)width / (float)height;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
            glm::mat4 view = scene.camera.getViewMatrix();

            // 1. SHADOW PASS: Render scene from light's perspective
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

            // 2. G-BUFFER PASS: Render geometry data for SSGI
            gBufferShader.use();
            gBufferShader.setMat4("view", view);
            gBufferShader.setMat4("projection", projection);

            ssgi.bindGBufferForWriting();
            for (const auto& entity : scene.entities) {
                auto meshComp = entity->getComponent<MeshComponent>();
                if (meshComp) {
                    auto transform = entity->getComponent<TransformComponent>();
                    if (transform) {
                        gBufferShader.setMat4("model", transform->getModelMatrix());
                        gBufferShader.setVec3("objectColor", meshComp->color);
                        meshComp->mesh->Draw(gBufferShader.ID);
                    }
                }
            }

            // 3. SSGI PASS: Calculate global illumination
            glDisable(GL_DEPTH_TEST);
            
            ssgiShader.use();
            ssgiShader.setMat4("projection", projection);
            ssgiShader.setMat4("view", view);
            ssgiShader.setFloat("ssgiRadius", ssgi.ssgiRadius);
            ssgiShader.setFloat("ssgiIntensity", ssgi.ssgiIntensity);
            ssgiShader.setFloat("ssgiMaxDistance", ssgi.ssgiMaxDistance);
            ssgiShader.setInt("ssgiSamples", ssgi.ssgiSamples);
            ssgiShader.setVec2("screenSize", glm::vec2(width, height));
            ssgiShader.setFloat("time", glfwGetTime());
            
            // Bind G-Buffer textures
            ssgiShader.setInt("gPosition", 0);
            ssgiShader.setInt("gNormal", 1);
            ssgiShader.setInt("gAlbedo", 2);
            ssgiShader.setInt("gDepth", 3);
            
            ssgi.bindSSGIForWriting();
            ssgi.bindForReading();
            quad.render();

            // 4. SSGI BLUR PASS: Blur the SSGI result
            ssgiBlurShader.use();
            ssgiBlurShader.setVec2("screenSize", glm::vec2(width, height));
            ssgiBlurShader.setInt("ssgiTexture", 4);
            ssgiBlurShader.setInt("gNormal", 1);
            ssgiBlurShader.setInt("gPosition", 0);
            
            // Horizontal blur
            ssgiBlurShader.setBool("horizontal", true);
            ssgi.bindSSGIBlurForWriting();
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ssgi.ssgiTexture);
            quad.render();
            
            // Vertical blur
            ssgiBlurShader.setBool("horizontal", false);
            ssgi.bindSSGIForWriting();
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ssgi.ssgiBlurTexture);
            quad.render();

            // 5. FINAL COMPOSITE PASS: Combine direct lighting with SSGI
            compositeShader.use();
            compositeShader.setMat4("view", view);
            compositeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            compositeShader.setVec3("lightPos", lightPos);
            compositeShader.setVec3("lightColor", lightColor);
            compositeShader.setVec3("viewPos", scene.camera.position);
            compositeShader.setFloat("ssgiStrength", 1.5f);
            compositeShader.setFloat("ambientStrength", 0.15f);
            
            // Bind textures
            compositeShader.setInt("gPosition", 0);
            compositeShader.setInt("gNormal", 1);
            compositeShader.setInt("gAlbedo", 2);
            compositeShader.setInt("ssgiTexture", 4);
            compositeShader.setInt("shadowMap", 5);
            
            // Render to screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, width, height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            shadowMap.bindForReading(5);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ssgi.ssgiTexture);
            ssgi.bindForReading();
            quad.render();
            
            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
            glm::mat4 ortho = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
            std::string fpsText = "FPS: " + std::to_string(fps);
            textRenderer.RenderText(fpsText, 10.0f, static_cast<float>(height) - 30.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), ortho);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);

            window.swapBuffers();
            window.pollEvents();
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}

// Callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window, Camera& camera, Scene& scene, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(3, deltaTime);

    // Light movement
    float lightSpeed = 3.0f * deltaTime;
    
    // Find the light entity and update its position
    for (const auto& entity : scene.entities) {
        if (auto light = entity->getComponent<LightComponent>()) {
            if (auto transform = entity->getComponent<TransformComponent>()) {
                // Arrow keys for X/Z movement
                if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                    transform->position.x -= lightSpeed;
                if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                    transform->position.x += lightSpeed;
                if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                    transform->position.z -= lightSpeed;
                if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                    transform->position.z += lightSpeed;
                
                // K/L keys for Y movement (up/down)
                if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
                    transform->position.y += lightSpeed;
                if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
                    transform->position.y -= lightSpeed;
                
                break; // Only move the first light found
            }
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    static bool firstMouse = true;
    static float lastX = 800.0f / 2.0f;
    static float lastY = 600.0f / 2.0f;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->processMouse(xoffset, yoffset);
    }
}
