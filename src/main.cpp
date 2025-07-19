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
 * - V: Toggle main light on/off
 * - G: Toggle global illumination
 * - T: Toggle SSAO
 * - F: Toggle screen space reflections
 * - C: Cycle anti-aliasing (None/FXAA/TAA)
 * - Z: Cycle quality levels
 * - R: Reset temporal accumulation
 * - X: Show performance breakdown
 * - Space: Pause/unpause
 * - ESC: Exit
 */

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <future>
#include <unordered_map>
#include <chrono>
#include <cfloat>
#include <iomanip>

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
#include "../scripts/Behaviour.h"
#include "../scripts/RotationComponent.h"

// Advanced rendering features
#include "../include/ShadowMap.h"
#include "../include/FullscreenQuad.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../include/RadianceCascades.h"

#include <string>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>
#include <algorithm>

// Build information
#define BUILD_NUMBER "24001"
#define BUILD_DATE __DATE__

// Enhanced performance profiler with detailed logging
class PerformanceProfiler {
private:
    struct TimingData {
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        float lastTime = 0.0f;
        float minTime = FLT_MAX;
        float maxTime = 0.0f;
        float avgTime = 0.0f;
        int sampleCount = 0;
        
        void updateStats(float newTime) {
            lastTime = newTime;
            minTime = std::min(minTime, newTime);
            maxTime = std::max(maxTime, newTime);
            
            // Rolling average
            sampleCount++;
            float alpha = std::min(1.0f / sampleCount, 0.1f); // Converge to 10-sample average
            avgTime = avgTime * (1.0f - alpha) + newTime * alpha;
        }
    };
    
    std::unordered_map<std::string, TimingData> cpuTimers;
    std::mutex timerMutex;
    int frameCounter = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> frameStart;
    
public:
    void beginFrame() {
        frameStart = std::chrono::high_resolution_clock::now();
        frameCounter++;
    }
    
    void beginTimer(const std::string& name) {
        std::lock_guard<std::mutex> lock(timerMutex);
        cpuTimers[name].start = std::chrono::high_resolution_clock::now();
    }
    
    void endTimer(const std::string& name) {
        auto end = std::chrono::high_resolution_clock::now();
        std::lock_guard<std::mutex> lock(timerMutex);
        
        auto& timer = cpuTimers[name];
        float elapsed = std::chrono::duration<float, std::milli>(end - timer.start).count();
        timer.updateStats(elapsed);
    }
    
    void logDetailedStats() {
        std::lock_guard<std::mutex> lock(timerMutex);
        
        std::cout << "\n=== PERFORMANCE BREAKDOWN (Frame " << frameCounter << ") ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        // Sort by average time (highest first)
        std::vector<std::pair<std::string, TimingData*>> sortedTimers;
        for (auto& [name, data] : cpuTimers) {
            sortedTimers.push_back({name, &data});
        }
        std::sort(sortedTimers.begin(), sortedTimers.end(), 
            [](const auto& a, const auto& b) { return a.second->avgTime > b.second->avgTime; });
        
        float totalTime = 0.0f;
        for (const auto& [name, data] : sortedTimers) {
            totalTime += data->avgTime;
        }
        
        for (const auto& [name, data] : sortedTimers) {
            float percentage = (data->avgTime / totalTime) * 100.0f;
            std::cout << std::setw(20) << name << ": " 
                     << std::setw(6) << data->avgTime << "ms avg (" 
                     << std::setw(5) << percentage << "%) [" 
                     << std::setw(6) << data->minTime << " - " 
                     << std::setw(6) << data->maxTime << "ms]" << std::endl;
        }
        
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        std::cout << std::setw(20) << "TOTAL_FRAME" << ": " 
                 << std::setw(6) << frameTime << "ms" << std::endl;
        std::cout << std::setw(20) << "TARGET_60FPS" << ": " 
                 << std::setw(6) << "16.67ms (current: " << (1000.0f / frameTime) << " fps)" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
    
    float getLastTime(const std::string& name) {
        std::lock_guard<std::mutex> lock(timerMutex);
        auto it = cpuTimers.find(name);
        return (it != cpuTimers.end()) ? it->second.lastTime : 0.0f;
    }
};

// Input processing thread data
struct InputData {
    std::atomic<bool> moveForward{false};
    std::atomic<bool> moveBackward{false};
    std::atomic<bool> moveLeft{false};
    std::atomic<bool> moveRight{false};
    std::atomic<bool> ambientToggle{false};
    std::atomic<bool> giToggle{false};
    std::atomic<bool> ssaoToggle{false};
    std::atomic<bool> lightToggle{false};      // V key - toggle main light
    std::atomic<bool> qualityToggle{false};
    std::atomic<bool> resetTemporal{false};
    std::atomic<bool> pauseToggle{false};
    std::atomic<bool> exitRequested{false};
    std::atomic<bool> ssrToggle{false};        // F key - toggle SSR
    std::atomic<bool> antiAliasingToggle{false}; // C key - cycle AA modes
    std::atomic<bool> showPerformance{false};  // X key - show performance breakdown
    
    // Light controls
    std::atomic<float> lightMoveX{0.0f};
    std::atomic<float> lightMoveZ{0.0f};
    std::atomic<float> lightMoveY{0.0f};
    std::atomic<float> lightIntensityDelta{0.0f};
    std::atomic<float> lightRadiusDelta{0.0f};
    
    std::mutex mouseMutex;
    double mouseX = 0.0, mouseY = 0.0;
    bool mouseUpdated = false;
};

// Simplified performance monitoring
struct PerformanceData {
    std::atomic<int> fps{0};
    std::atomic<float> frameTime{0.0f};
    std::atomic<float> shadowTime{0.0f};
    std::atomic<float> gbufferTime{0.0f};
    std::atomic<float> ssaoTime{0.0f};
    std::atomic<float> giTime{0.0f};
    std::atomic<float> compositeTime{0.0f};
    std::atomic<float> uiTime{0.0f};
    std::atomic<bool> giEnabled{true};
    std::atomic<bool> ssaoEnabled{false};
    std::atomic<int> qualityLevel{2};
    
    std::mutex textMutex;
    bool textReady = true;
};

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Multithreading function prototypes
void inputProcessingThread(GLFWwindow* window, InputData& inputData, std::atomic<bool>& running);
void performanceMonitoringThread(PerformanceData& perfData, std::atomic<bool>& running);

// Global variables for mouse input handling
bool firstMouse = true;
float lastX = 1280.0f / 2.0;
float lastY = 800.0f / 2.0;

// Asynchronous input processing thread function
void inputProcessingThread(GLFWwindow* window, InputData& inputData, std::atomic<bool>& running) {
    while (running) {
        // Reset movement states each frame
        inputData.moveForward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        inputData.moveBackward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        inputData.moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        inputData.moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        inputData.exitRequested = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        
        // Light movement controls - accumulate over time
        float lightSpeed = 0.05f; // Reduced for smoother control
        inputData.lightMoveX = 0.0f;
        inputData.lightMoveZ = 0.0f;
        inputData.lightMoveY = 0.0f;
        inputData.lightIntensityDelta = 0.0f;
        inputData.lightRadiusDelta = 0.0f;
        
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) inputData.lightMoveX = -lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) inputData.lightMoveX = lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) inputData.lightMoveZ = -lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) inputData.lightMoveZ = lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) inputData.lightMoveY = lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) inputData.lightMoveY = -lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) inputData.lightIntensityDelta = lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) inputData.lightIntensityDelta = -lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) inputData.lightRadiusDelta = lightSpeed;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) inputData.lightRadiusDelta = -lightSpeed;
        
        // Toggle states (handled with static debouncing)
        static bool lastM = false, lastG = false, lastT = false, lastV = false, lastZ = false, lastR = false, lastSpace = false, lastF = false, lastC = false, lastX = false;
        
        bool currentM = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        if (!lastM && currentM) inputData.ambientToggle = true;
        lastM = currentM;
        
        bool currentG = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
        if (!lastG && currentG) inputData.giToggle = true;
        lastG = currentG;
        
        bool currentT = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (!lastT && currentT) inputData.ssaoToggle = true;
        lastT = currentT;
        
        bool currentV = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
        if (!lastV && currentV) inputData.lightToggle = true;
        lastV = currentV;
        
        bool currentZ = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        if (!lastZ && currentZ) inputData.qualityToggle = true;
        lastZ = currentZ;
        
        bool currentR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (!lastR && currentR) inputData.resetTemporal = true;
        lastR = currentR;
        
        bool currentSpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (!lastSpace && currentSpace) inputData.pauseToggle = true;
        lastSpace = currentSpace;
        
        bool currentF = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (!lastF && currentF) inputData.ssrToggle = true;
        lastF = currentF;
        
        bool currentC = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        if (!lastC && currentC) inputData.antiAliasingToggle = true;
        lastC = currentC;
        
        bool currentX = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
        if (!lastX && currentX) inputData.showPerformance = true;
        lastX = currentX;
        
        // Run at 120 Hz for responsive input
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

// Simplified performance monitoring thread (reduced overhead after fixing UI bottleneck)
void performanceMonitoringThread(PerformanceData& perfData, std::atomic<bool>& running) {
    while (running) {
        // Just keep the data updated - UI rendering is now optimized
        
        // Run at lower frequency since UI is now optimized
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * Main rendering loop and application entry point
 * 
 * Initializes the rendering system, sets up the complete graphics pipeline,
 * and runs the main game loop with real-time global illumination.
 */
int main() {
    // Multithreading setup (declared outside try block for proper cleanup)
    InputData inputData;
    std::atomic<bool> inputThreadRunning{false};
    std::thread inputThread;
    
    PerformanceData perfData;
    std::atomic<bool> perfThreadRunning{false};
    std::thread perfThread;
    
    try {
        // Initialize main window with OpenGL context
        Window window(1280, 800, "Vibe-GI: Global Illumination Renderer");

        // Set up window callbacks for input handling
        glfwSetFramebufferSizeCallback(window.getGLFWWindow(), framebuffer_size_callback);
        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window.getGLFWWindow(), mouse_callback);

        // Enable depth testing for proper 3D rendering
        glEnable(GL_DEPTH_TEST);
        
        // Disable vsync for maximum performance
        glfwSwapInterval(0);

        // Initialize all shaders for the rendering pipeline
        Shader lightingShader("shaders/lighting.vert", "shaders/lighting.frag");           // Forward lighting (legacy)
        Shader shadowShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");     // Shadow map generation
        Shader gBufferShader("shaders/gbuffer.vert", "shaders/gbuffer.frag");             // Deferred geometry pass
        Shader rcShader("shaders/fullscreen.vert", "shaders/rc_cascade.frag");            // Radiance cascades computation
        Shader blurShader("shaders/fullscreen.vert", "shaders/blur.frag");                // GI temporal blur
        Shader compositeShader("shaders/fullscreen.vert", "shaders/final_composite.frag"); // Final lighting composite

        Shader copyShader("shaders/fullscreen.vert", "shaders/copy.frag");               // Direct copy (no AA)
        Shader ssaoShader("shaders/fullscreen.vert", "shaders/ssao.frag");                // Screen-space ambient occlusion
        Shader ssaoBlurShader("shaders/fullscreen.vert", "shaders/ssao_blur.frag");       // SSAO blur for noise reduction
        Shader ssrShader("shaders/fullscreen.vert", "shaders/ssr.frag");                  // Screen-space reflections
        Shader taaShader("shaders/fullscreen.vert", "shaders/taa.frag");                  // Temporal anti-aliasing
        Shader fxaaShader("shaders/fullscreen.vert", "shaders/fxaa.frag");                // Fast approximate anti-aliasing
        // Initialize ImGui for ultra-fast UI rendering
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup ImGui style
        ImGui::StyleColorsDark();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window.getGLFWWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // Initialize core rendering systems
        ShadowMap shadowMap;                                    // Directional light shadow mapping
        RadianceCascades rc(1280, 800, 6);                     // 6-cascade radiance cascade GI system (high quality)
        FullscreenQuad quad;                                    // Fullscreen quad for post-processing

        // Create offscreen framebuffer for composite pass (before TAA)
        // This allows us to apply temporal anti-aliasing as a final step
        unsigned int compositeFBO;
        unsigned int compositeTexture;
        glGenFramebuffers(1, &compositeFBO);
        glGenTextures(1, &compositeTexture);
        glBindTexture(GL_TEXTURE_2D, compositeTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 800, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
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
        // Main rendering settings and toggles
        bool ambientEnabled = false;    // Toggle for ambient lighting (default off)
        bool giEnabled = true;          // Toggle for global illumination (default on)
        bool ssaoEnabled = false;       // Toggle for screen space ambient occlusion (default off)
        bool ssrEnabled = false;        // Toggle for screen space reflections (default off)
        bool lightEnabled = true;       // Toggle for main light (default on)
        bool paused = false;            // Toggle for pause state
        float pausedTime = 0.0f;        // Time accumulator for pause system
        int antiAliasingMode = 2;       // AA mode: 0=none, 1=FXAA, 2=TAA (default TAA)
        int qualityLevel = 2;           // Quality level: 0=super low, 1=performance, 2=balanced, 3=high, 4=ultra

        float deltaTime = 0.0f;         // Frame time delta
        float lastFrame = 0.0f;         // Previous frame timestamp
        int frameCount = 0;             // Frame counter for FPS calculation
        float fpsTimer = 0.0f;          // FPS calculation timer
        int fps = 0;                    // Current FPS
        static int lastWidth = 0;       // Previous frame width (for resize detection)
        static int lastHeight = 0;      // Previous frame height (for resize detection)

        // Enhanced performance profiler
        PerformanceProfiler profiler;
        
        // Start input processing thread
        inputThreadRunning = true;
        inputThread = std::thread(inputProcessingThread, window.getGLFWWindow(), std::ref(inputData), std::ref(inputThreadRunning));
        
        // Start performance monitoring thread
        perfThreadRunning = true;
        perfThread = std::thread(performanceMonitoringThread, std::ref(perfData), std::ref(perfThreadRunning));
        
        // Performance monitoring (updated async from GPU)
        float frameTime = 0.0f;
        float shadowTime = 0.0f;
        float gbufferTime = 0.0f;
        float ssaoTime = 0.0f;
        float giTime = 0.0f;
        float compositeTime = 0.0f;

        /**
         * MAIN RENDER LOOP
         * 
         * This loop implements the complete real-time rendering pipeline with
         * global illumination, running at interactive frame rates.
         */
        while (!window.shouldClose()) {
            // Begin detailed frame profiling
            profiler.beginFrame();
            
            profiler.beginTimer("frame_setup");
            // Calculate frame timing for smooth animation and movement
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // Update FPS counter
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                fps = static_cast<int>(frameCount / fpsTimer);
                perfData.fps = fps; // Feed to performance thread
                frameCount = 0;
                fpsTimer -= 1.0f;
            }
            profiler.endTimer("frame_setup");
            
            // UI cache variables - accessible from both input handling and UI rendering
            static int uiFrameCounter = 0;
            static std::string cachedFpsText = "FPS: 0";
            static std::string cachedQualityText = "Quality: Balanced (3C)";
            static std::string cachedGiStatusText = "GI: ON";
            static std::string cachedSsaoStatusText = "SSAO: ON";
            static std::string cachedSsrStatusText = "SSR: OFF";
            static std::string cachedAaStatusText = "AA: TAA";
            uiFrameCounter++;
            
            profiler.beginTimer("input_processing");
            // Process input from async thread
            if (inputData.exitRequested) {
                glfwSetWindowShouldClose(window.getGLFWWindow(), true);
            }
            
            // Handle toggle states
            if (inputData.ambientToggle.exchange(false)) {
                ambientEnabled = !ambientEnabled;
            }
            if (inputData.giToggle.exchange(false)) {
                giEnabled = !giEnabled;
                perfData.giEnabled = giEnabled; // Update performance thread
                
                // Immediately update UI cache for responsive feedback
                cachedGiStatusText = "GI: " + std::string(giEnabled ? "ON" : "OFF");
            }
            if (inputData.ssaoToggle.exchange(false)) {
                ssaoEnabled = !ssaoEnabled;
                perfData.ssaoEnabled = ssaoEnabled; // Update performance thread
                
                // Immediately update UI cache for responsive feedback
                cachedSsaoStatusText = "SSAO: " + std::string(ssaoEnabled ? "ON" : "OFF");
            }
            if (inputData.lightToggle.exchange(false)) {
                lightEnabled = !lightEnabled;
                // Reset temporal accumulation when light state changes
                rc.resetTemporalAccumulation();
            }
            if (inputData.ssrToggle.exchange(false)) {
                ssrEnabled = !ssrEnabled;
                cachedSsrStatusText = "SSR: " + std::string(ssrEnabled ? "ON" : "OFF");
            }
            if (inputData.antiAliasingToggle.exchange(false)) {
                antiAliasingMode = (antiAliasingMode + 1) % 3; // Cycle: None -> FXAA -> TAA -> None
                std::string aaNames[] = {"None", "FXAA", "TAA"};
                cachedAaStatusText = "AA: " + aaNames[antiAliasingMode];
            }
            if (inputData.qualityToggle.exchange(false)) {
                qualityLevel = (qualityLevel + 1) % 5; // 5 quality levels: 0-4
                perfData.qualityLevel = qualityLevel; // Update performance thread
                
                // Immediately update UI cache for responsive feedback
                std::string qualityNames[] = {"Super Low", "Performance", "Balanced", "High", "Ultra"};
                std::string cascadeCounts[] = {"2C", "3C", "4C", "5C", "6C"};
                cachedQualityText = "Quality: " + qualityNames[qualityLevel] + " (" + cascadeCounts[qualityLevel] + ")";
            }

            if (inputData.resetTemporal.exchange(false)) {
                rc.resetTemporalAccumulation();
            }
            if (inputData.pauseToggle.exchange(false)) {
                paused = !paused;
                if (paused) {
                    pausedTime = glfwGetTime();
                    glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            
            // Process camera movement (only when not paused)
            if (!paused) {
                bool anyMovement = false;
                if (inputData.moveForward) { scene.camera.processKeyboard(0, deltaTime); anyMovement = true; }
                if (inputData.moveBackward) { scene.camera.processKeyboard(1, deltaTime); anyMovement = true; }
                if (inputData.moveLeft) { scene.camera.processKeyboard(2, deltaTime); anyMovement = true; }
                if (inputData.moveRight) { scene.camera.processKeyboard(3, deltaTime); anyMovement = true; }
                
                // Reset temporal accumulation immediately on any movement input
                if (anyMovement) {
                    rc.resetTemporalAccumulation();
                }
            }

            // Process light controls from async input (only when not paused)
            if (!paused) {
                bool anyLightMovement = false;
                for (const auto& entity : scene.entities) {
                    if (auto light = entity->getComponent<LightComponent>()) {
                        if (auto transform = entity->getComponent<TransformComponent>()) {
                            // Apply light movement from async input
                            if (inputData.lightMoveX != 0.0f || inputData.lightMoveZ != 0.0f || inputData.lightMoveY != 0.0f) {
                                transform->position.x += inputData.lightMoveX * deltaTime * 60.0f; // Scale by frame rate
                                transform->position.z += inputData.lightMoveZ * deltaTime * 60.0f;
                                transform->position.y += inputData.lightMoveY * deltaTime * 60.0f;
                                anyLightMovement = true;
                            }
                            
                            // Apply light property changes
                            if (inputData.lightIntensityDelta != 0.0f) {
                                light->intensity += inputData.lightIntensityDelta * deltaTime * 60.0f;
                                light->intensity = std::max(0.0f, light->intensity);
                                anyLightMovement = true;
                            }
                            
                            if (inputData.lightRadiusDelta != 0.0f) {
                                light->radius += inputData.lightRadiusDelta * deltaTime * 60.0f;
                                light->radius = std::max(0.5f, light->radius);
                                anyLightMovement = true;
                            }
                        }
                    }
                }
                
                // Reset temporal accumulation immediately on any light changes
                if (anyLightMovement) {
                    rc.resetTemporalAccumulation();
                }
                
                // Update all behaviour components (only when not paused)
                for (const auto& entity : scene.entities) {
                    if (auto behaviour = entity->getComponent<Behaviour>()) {
                        if (!behaviour->hasStarted()) {
                            behaviour->Start();
                            behaviour->markStarted();
                        }
                        behaviour->Update(deltaTime);
                    }
                }
            }
            profiler.endTimer("input_processing");
            
            profiler.beginTimer("scene_setup");
            // Extract light information from ECS for rendering
            // In a real engine, this would support multiple lights
            glm::vec3 lightPos(0.0f);
            glm::vec3 lightColor(1.0f);
            float lightRadius = 2.0f; // Default radius for light attenuation
            static glm::vec3 lastLightPos(0.0f);
            static bool firstFrame = true;
            
            // Find the primary light in the scene
            for (const auto& entity : scene.entities) {
                if (auto light = entity->getComponent<LightComponent>()) {
                    if (auto transform = entity->getComponent<TransformComponent>()) {
                        lightPos = transform->position;
                        // Apply light toggle - when disabled, lightColor becomes (0,0,0)
                        if (lightEnabled) {
                            lightColor = light->color * light->intensity;
                        } else {
                            lightColor = glm::vec3(0.0f, 0.0f, 0.0f);
                        }
                        lightRadius = light->radius;
                    }
                }
            }
            
            // Initialize light position tracking on first frame
            if (firstFrame) {
                lastLightPos = lightPos;
                firstFrame = false;
            }
            
            // Reset temporal accumulation if camera moved significantly
            // This prevents ghosting artifacts when camera moves
            static glm::vec3 lastCameraPos(0.0f);
            static glm::vec3 lastCameraDirection(0.0f);
            static bool firstCameraFrame = true;
            
            glm::vec3 currentCameraDirection = scene.camera.front;
            float cameraMovement = glm::length(scene.camera.position - lastCameraPos);
            float cameraRotation = 1.0f - glm::dot(currentCameraDirection, lastCameraDirection);
            
            if (firstCameraFrame) {
                lastCameraPos = scene.camera.position;
                lastCameraDirection = currentCameraDirection;
                firstCameraFrame = false;
            } else if (cameraMovement > 0.05f || cameraRotation > 0.01f) { // Much more sensitive - any camera movement resets
                rc.resetTemporalAccumulation();
                lastCameraPos = scene.camera.position;
                lastCameraDirection = currentCameraDirection;
            }
            
            // Reset temporal accumulation if light moved significantly
            // This prevents ghosting artifacts when lighting changes rapidly
            float lightMovement = glm::length(lightPos - lastLightPos);
            if (lightMovement > 0.01f) { // Much more sensitive threshold for light movement
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

            // No more jittering - clean, stable rendering
            
            // Store previous frame matrices (for potential future effects)
            static glm::mat4 previousView = view;
            static glm::mat4 previousProjection = projection;

            // Poll window events (input, resize, etc.)
            window.pollEvents();
            profiler.endTimer("scene_setup");

            /**
             * RENDERING PIPELINE - Multi-pass deferred rendering with global illumination
             */

            profiler.beginTimer("rendering_pipeline");
            auto passStart = std::chrono::high_resolution_clock::now();

            // PASS 1: SHADOW MAP GENERATION
            // Render scene from light's perspective to generate shadow map
            profiler.beginTimer("shadow_total");
            profiler.beginTimer("shadow_setup");
            shadowShader.use();
            shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shadowMap.bindForWriting();
            profiler.endTimer("shadow_setup");
            
            profiler.beginTimer("shadow_render");
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
            profiler.endTimer("shadow_render");
            profiler.endTimer("shadow_total");
            
            shadowTime = profiler.getLastTime("shadow_total");
            perfData.shadowTime = shadowTime; // Feed to performance thread

            // PASS 2: G-BUFFER GENERATION (Deferred Rendering)
            // Render geometry data (position, normal, albedo, motion vectors) to textures
            profiler.beginTimer("gbuffer_total");
            profiler.beginTimer("gbuffer_setup");
            
            rc.bindGBufferForWriting();
            gBufferShader.use();
            gBufferShader.setMat4("projection", projection);
            gBufferShader.setMat4("view", view);
            gBufferShader.setMat4("previousProjection", previousProjection);
            gBufferShader.setMat4("previousView", previousView);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            profiler.endTimer("gbuffer_setup");

            // Render all scene geometry to G-buffer
            profiler.beginTimer("gbuffer_render");
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
            profiler.endTimer("gbuffer_render");
            profiler.endTimer("gbuffer_total");
            
            gbufferTime = profiler.getLastTime("gbuffer_total");
            perfData.gbufferTime = gbufferTime; // Feed to performance thread
            
            // PASS 3: SCREEN SPACE AMBIENT OCCLUSION (SSAO)
            // Compute ambient occlusion for enhanced depth perception (if enabled)
            // Quality-dependent SSAO: disabled for super low, enabled for others
            profiler.beginTimer("ssao_total");
            
            if (ssaoEnabled && qualityLevel > 0) {
                profiler.beginTimer("ssao_compute");
                rc.computeSSAO(ssaoShader, projection);
                profiler.endTimer("ssao_compute");
                
                // PASS 4: SSAO BLUR
                // Quality-dependent blur: skip for performance level, full for others
                profiler.beginTimer("ssao_blur");
                if (qualityLevel > 1) {
                    rc.blurSSAO(ssaoBlurShader);
                }
                profiler.endTimer("ssao_blur");
            }
            profiler.endTimer("ssao_total");
            
            ssaoTime = profiler.getLastTime("ssao_total");
            perfData.ssaoTime = ssaoTime; // Feed to performance thread
            
            // 5-Level Quality System with increased cascade counts for high-end hardware
            // Super Low (0): 2 cascades,  minimal GI but still good quality
            // Performance (1): 3 cascades, good GI quality with performance focus
            // Balanced (2): 4 cascades, excellent balance of quality/performance  
            // High (3): 5 cascades, high quality GI for detailed scenes
            // Ultra (4): 6 cascades, maximum quality GI for ultimate fidelity
            int activeCascades = 0;
            if (giEnabled) {
                switch (qualityLevel) {
                    case 0: activeCascades = 2; break; // Super Low
                    case 1: activeCascades = 3; break; // Performance  
                    case 2: activeCascades = 4; break; // Balanced
                    case 3: activeCascades = 5; break; // High
                    case 4: activeCascades = 6; break; // Ultra (maximum quality)
                    default: activeCascades = 4; break; // Fallback
                }
            }
            // Clamp to valid range for safety
            activeCascades = std::max(0, std::min(activeCascades, 6));
            
            // PASS 5: RADIANCE CASCADES GLOBAL ILLUMINATION
            // Compute multi-bounce indirect lighting using radiance cascades
            profiler.beginTimer("gi_total");
            
            if (giEnabled) {
                profiler.beginTimer("gi_setup");
                rcShader.use();
                rcShader.setMat4("invView", glm::inverse(view)); // For world space calculations
                rcShader.setVec3("lightPos", lightPos);          // World space light position
                rcShader.setVec3("lightColor", lightColor);      // Light color and intensity
                rcShader.setFloat("lightRadius", lightRadius);   // Light attenuation radius
                rcShader.setFloat("time", glfwGetTime());         // Time for temporal effects
                rcShader.setInt("activeCascades", activeCascades); // Dynamic cascade count for quality-aware computation
                profiler.endTimer("gi_setup");
                
                profiler.beginTimer("gi_compute");
                rc.compute(rcShader, view, projection, activeCascades);
                profiler.endTimer("gi_compute");
                
                // PASS 6: GI QUALITY-DEPENDENT BLUR
                // Apply blur based on quality level for optimal performance/quality balance
                profiler.beginTimer("gi_blur");
                switch (qualityLevel) {
                    case 0: // Super Low: minimal blur for performance
                        // Skip blur entirely for maximum performance
                        break;
                    case 1: // Performance: reduced blur (only blur first 2 cascades)
                        if (activeCascades >= 2) {
                            rc.blur(blurShader, 2); // Only blur first 2 cascades for performance
                        } else {
                            rc.blur(blurShader, activeCascades);
                        }
                        break;
                    case 2: // Balanced: standard blur
                        rc.blur(blurShader, activeCascades);
                        break;
                    case 3: // High: enhanced blur
                        rc.blur(blurShader, activeCascades);
                        break;
                    case 4: // Ultra: maximum quality blur (could be multi-pass in future)
                        rc.blur(blurShader, activeCascades);
                        break;
                }
                profiler.endTimer("gi_blur");
            }
            profiler.endTimer("gi_total");
            
            giTime = profiler.getLastTime("gi_total");
            perfData.giTime = giTime; // Feed to performance thread
            
            // PASS 7: FINAL COMPOSITE TO OFFSCREEN BUFFER
            // Combine all lighting contributions into final image
            profiler.beginTimer("composite_total");
            profiler.beginTimer("composite_setup");
            
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
            // CORRECTED GI strength: More cascades capture more light, so need LOWER multipliers for visual consistency
            // Ultra mode has additional enhancements (multi-bounce, better upsampling) so needs even lower strength
            float giStrength = 0.0f;
            if (giEnabled) {
                switch (qualityLevel) {
                    case 0: giStrength = 0.85f; break; // Super Low (2C): highest strength since fewer cascades
                    case 1: giStrength = 0.70f; break; // Performance (3C): reduced strength for extra cascade
                    case 2: giStrength = 0.55f; break; // Balanced (4C): balanced strength for good coverage
                    case 3: giStrength = 0.45f; break; // High (5C): this looks good - keep as reference
                    case 4: giStrength = 0.82f; break; // Ultra (6C): increased to match High's effective brightness (0.82 × 0.22 = 0.18)
                    default: giStrength = 0.55f; break; // Fallback to balanced
                }
            }
            compositeShader.setFloat("ssgiStrength", giStrength);
            compositeShader.setFloat("ambientStrength", ambientEnabled ? 0.08f : 0.0f); // Reduced ambient
            compositeShader.setFloat("ssaoStrength", (ssaoEnabled && qualityLevel > 0) ? 1.0f : 0.0f); // Conditional SSAO contribution
            compositeShader.setInt("activeCascades", activeCascades); // Pass cascade count for quality-aware processing
            
            // Bind all G-buffer textures for lighting calculations
            compositeShader.setInt("gPosition", 0);
            compositeShader.setInt("gNormal", 1);
            compositeShader.setInt("gAlbedo", 2);
            compositeShader.setInt("gEmission", 11); // New: emission texture
            compositeShader.setInt("shadowMap", 3);
            compositeShader.setInt("ssaoTexture", 10);
            
            // Bind radiance cascade textures (multi-scale GI data) - only active cascades
            for (int i = 0; i < activeCascades; ++i) {
                compositeShader.setInt("rcTexture[" + std::to_string(i) + "]", 4 + i);
            }
            // Ensure unused cascade slots are set to safe values
            for (int i = activeCascades; i < 6; ++i) {
                compositeShader.setInt("rcTexture[" + std::to_string(i) + "]", 0); // Bind to position texture as safe fallback
            }
            compositeShader.setInt("activeCascades", activeCascades);
            profiler.endTimer("composite_setup");
            
            // Activate and bind all required textures
            profiler.beginTimer("composite_textures");
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
            
            // Bind emission texture
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, rc.getGEmission());
            
            // Bind all cascade textures for GI sampling - only active cascades
            for (int i = 0; i < activeCascades; ++i) {
                glActiveTexture(GL_TEXTURE4 + i);
                glBindTexture(GL_TEXTURE_2D, rc.getTexture(i));
            }
            // Bind safe fallback textures to unused cascade slots
            for (int i = activeCascades; i < 6; ++i) {
                glActiveTexture(GL_TEXTURE4 + i);
                glBindTexture(GL_TEXTURE_2D, rc.getGPosition()); // Safe fallback texture
            }

            profiler.endTimer("composite_textures");

            // Render fullscreen quad to perform lighting calculations
            profiler.beginTimer("composite_render");
            quad.render();
            profiler.endTimer("composite_render");
            profiler.endTimer("composite_total");
            
            compositeTime = profiler.getLastTime("composite_total");
            perfData.compositeTime = compositeTime; // Feed to performance thread
            
            glEnable(GL_DEPTH_TEST);

            // PASS 8: SCREEN SPACE REFLECTIONS (Optional)
            if (ssrEnabled) {
                profiler.beginTimer("ssr_total");
                rc.computeSSR(ssrShader, compositeTexture, view, projection, scene.camera.position);
                profiler.endTimer("ssr_total");
            }

            // PASS 9: ANTI-ALIASING (Optional) - DISABLED TO FIX TEXTURE ISSUE
            unsigned int finalTexture = compositeTexture;
            
            // TEMPORARILY DISABLE AA TO ISOLATE TEXTURE ISSUE
            // PASS 9: ANTI-ALIASING (FXAA or TAA)
            if (antiAliasingMode == 1) { // FXAA
                profiler.beginTimer("fxaa_total");
                rc.applyFXAA(fxaaShader, finalTexture);
                finalTexture = rc.getTAATexture(); // Reuse TAA texture for FXAA output
                profiler.endTimer("fxaa_total");
            } else if (antiAliasingMode == 2) { // TAA
                profiler.beginTimer("taa_total");
                glm::mat4 currentViewProj = projection * view;
                static glm::mat4 previousViewProj = currentViewProj;
                rc.applyTAA(taaShader, finalTexture, currentViewProj, previousViewProj);
                finalTexture = rc.getTAATexture();
                previousViewProj = currentViewProj;
                profiler.endTimer("taa_total");
            }

            // PASS 10: FINAL OUTPUT TO SCREEN
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            copyShader.use();
            copyShader.setInt("inputTexture", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, finalTexture);

            quad.render();

            glEnable(GL_DEPTH_TEST);

            // PASS 9: FULLY STABLE UI RENDERING (NO FLICKERING)
            // All text always visible, with cached strings updated at different frequencies
            profiler.beginTimer("ui_total");
            
            profiler.beginTimer("ui_setup");
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            profiler.endTimer("ui_setup");
            
            // PROBE 1: Update FPS every 3 frames (high frequency for responsiveness)
            profiler.beginTimer("ui_cache_update");
            if (uiFrameCounter % 3 == 0) {
                cachedFpsText = "FPS: " + std::to_string(fps);
            }
            
            // PROBE 2: Update quality status every 6 frames (medium frequency)
            if (uiFrameCounter % 6 == 0) {
                std::string qualityNames[] = {"Super Low", "Performance", "Balanced", "High", "Ultra"};
                std::string cascadeCounts[] = {"2C", "3C", "4C", "5C", "6C"};
                cachedQualityText = "Quality: " + qualityNames[qualityLevel] + " (" + cascadeCounts[qualityLevel] + ")";
                cachedGiStatusText = "GI: " + std::string(giEnabled ? "ON" : "OFF");
                cachedSsaoStatusText = "SSAO: " + std::string(ssaoEnabled ? "ON" : "OFF");
            }
            profiler.endTimer("ui_cache_update");
            
            // Ultra-fast ImGui UI rendering
            profiler.beginTimer("ui_render");
            
            // Create main info overlay window
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.7f); // Transparent background
            if (ImGui::Begin("Vibe-GI Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Vibe-GI: Real-time Global Illumination");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Build %s (%s)", BUILD_NUMBER, BUILD_DATE);
                ImGui::Separator();
                
                ImGui::Text("WASD: Move camera");
                ImGui::Text("Mouse: Look around");
                ImGui::Text("M: Toggle Ambient, G: Toggle GI, T: Toggle SSAO");
                ImGui::Text("F: Toggle SSR, C: Cycle AA (None/FXAA/TAA)");
                ImGui::Text("Z: Quality Level (5 levels), X: Show Performance");
                ImGui::Text("Arrow Keys: Move Light, K/L: Height");
                ImGui::Text("O/P: Light Intensity, I/U: Light Radius");
                
                ImGui::Separator();
                
                // Status line with cached values
                ImGui::Text("%s", cachedFpsText.c_str());
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", cachedQualityText.c_str());
                ImGui::SameLine();
                ImGui::TextColored(giEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", cachedGiStatusText.c_str());
                ImGui::SameLine();
                ImGui::TextColored(ssaoEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", cachedSsaoStatusText.c_str());
                ImGui::TextColored(ssrEnabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", cachedSsrStatusText.c_str());
                ImGui::TextColored(antiAliasingMode > 0 ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", cachedAaStatusText.c_str());
            }
            ImGui::End();
            profiler.endTimer("ui_render");

            profiler.beginTimer("ui_cleanup");
            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            profiler.endTimer("ui_cleanup");
            profiler.endTimer("ui_total");
            
            // Store UI timing for performance monitoring
            float uiTime = profiler.getLastTime("ui_total");
            perfData.uiTime = uiTime; // Feed to performance thread

            // Store matrices for next frame
            previousView = view;
            previousProjection = projection;

            // Present final frame to screen
            profiler.beginTimer("buffer_swap");
            window.swapBuffers();
            profiler.endTimer("buffer_swap");
            
            profiler.endTimer("rendering_pipeline");
            
            // Calculate total frame time and log detailed statistics
            auto frameEnd = std::chrono::high_resolution_clock::now();
            frameTime = std::chrono::duration<float, std::milli>(frameEnd - passStart).count();
            perfData.frameTime = frameTime; // Feed to performance thread
            
            // Log detailed performance breakdown every 60 frames
            if (inputData.showPerformance.exchange(false)) { // Only log if requested
                profiler.logDetailedStats();
            }
        }

        // Clean up multithreading resources
        inputThreadRunning = false;
        inputThread.join();
        perfThreadRunning = false;
        if (perfThread.joinable()) {
            perfThread.join();
        }

        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        
        // Ensure threads are cleaned up even on error
        inputThreadRunning = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        perfThreadRunning = false;
        if (perfThread.joinable()) {
            perfThread.join();
        }
        
        // Cleanup ImGui even on error
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
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
 * Input processing function (DEPRECATED - now using threaded input)
 * 
 * Handles keyboard input for camera movement, light controls, and various toggles.
 * This function is called each frame and provides responsive input handling.
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
 * @param lightEnabled  Reference to main light toggle
 */
void processInput(GLFWwindow *window, Camera& camera, Scene& scene, RadianceCascades& rc, float deltaTime, bool& ambientEnabled, bool& giEnabled, bool& ssaoEnabled, bool& paused, float& pausedTime, int& qualityLevel, bool& lightEnabled) {
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
    
    // Toggle quality level with Z key (cycles: Super Low -> Performance -> Balanced -> High -> Ultra)
    static bool lastZ = false;
    bool currentZ = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
    if (!lastZ && currentZ) {
        qualityLevel = (qualityLevel + 1) % 5; // 5 quality levels: 0-4
    }
    lastZ = currentZ;
    
    // Reset temporal accumulation with R key (useful when lighting changes)
    static bool lastR = false;
    bool currentR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (!lastR && currentR) {
        rc.resetTemporalAccumulation();
    }
    lastR = currentR;
    
    // Toggle temporal accumulation entirely with Y key (to eliminate ghosting)
    static bool lastY = false;
    static bool temporalEnabled = true;
    bool currentY = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
    if (!lastY && currentY) {
        temporalEnabled = !temporalEnabled;
        rc.setTemporalAccumulation(temporalEnabled);
        if (!temporalEnabled) {
            rc.resetTemporalAccumulation(); // Clear any existing temporal data
        }
    }
    lastY = currentY;
    
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
