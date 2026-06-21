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
 * - 1-6: Scene selection (Cornell Box, Teapot, Stone Floor, Shadow Test, Default, Sponza)
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
 * - J: Toggle frustum culling
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
#include "../include/GLHeaders.h"
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
    std::atomic<bool> cullingToggle{false};    // J key - toggle frustum culling
    std::atomic<bool> samplingToggle{false};   // ? key - cycle sampling methods
    
    // Scene selection (1-6 keys)
    std::atomic<int> sceneSelection{-1};       // -1 = no selection, 0-5 = scene index
    
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



// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void window_focus_callback(GLFWwindow* window, int focused);

// Desired mouse-capture state. On Wayland, GLFW_CURSOR_DISABLED only engages once the
// window has pointer focus, so the actual cursor mode is (re)asserted from this flag in
// window_focus_callback whenever focus arrives (e.g. on startup click / alt-tab back).
std::atomic<bool> g_cursorCaptured{true};

// Multithreading function prototypes
void inputProcessingThread(GLFWwindow* window, InputData& inputData, std::atomic<bool>& running);

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
        static bool lastM = false, lastG = false, lastT = false, lastV = false, lastZ = false, lastR = false, lastSpace = false, lastF = false, lastC = false, lastX = false, lastJ = false;
        static bool last1 = false, last2 = false, last3 = false, last4 = false, last5 = false, last6 = false;
        
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
        bool currentQuestion = glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS; // '?' is usually shift + '/'
        if (!lastX && currentX) inputData.showPerformance = true;
        static bool lastQuestion = false;
        if (!lastQuestion && currentQuestion) inputData.samplingToggle = true;
        lastQuestion = currentQuestion;
        lastX = currentX;
        
        bool currentJ = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
        if (!lastJ && currentJ) inputData.cullingToggle = true;
        lastJ = currentJ;
        
        // Scene selection keys (1-6)
        bool current1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
        if (!last1 && current1) inputData.sceneSelection = 0; // Cornell Box
        last1 = current1;
        
        bool current2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
        if (!last2 && current2) inputData.sceneSelection = 1; // Teapot Lightbox
        last2 = current2;
        
        bool current3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
        if (!last3 && current3) inputData.sceneSelection = 2; // Stone Floor
        last3 = current3;
        
        bool current4 = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
        if (!last4 && current4) inputData.sceneSelection = 3; // Shadow Test
        last4 = current4;
        
        bool current5 = glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS;
        if (!last5 && current5) inputData.sceneSelection = 4; // Default Lightbox
        last5 = current5;
        
        bool current6 = glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS;
        if (!last6 && current6) inputData.sceneSelection = 5; // Sponza Overhead
        last6 = current6;
        
        // Run at 120 Hz for responsive input
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
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
    
    try {
        // Initialize main window with OpenGL context
        Window window(1280, 800, "Vibe-GI: Global Illumination Renderer");

        // Set up window callbacks for input handling
        glfwSetFramebufferSizeCallback(window.getGLFWWindow(), framebuffer_size_callback);
        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window.getGLFWWindow(), mouse_callback);
        glfwSetWindowFocusCallback(window.getGLFWWindow(), window_focus_callback);
        glfwFocusWindow(window.getGLFWWindow()); // ensure focus so cursor capture engages immediately

        // Enable depth testing for proper 3D rendering
        glEnable(GL_DEPTH_TEST);
        
        // Disable vsync for maximum performance
        glfwSwapInterval(0);

        // Initialize all shaders for the rendering pipeline
        Shader shadowShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");     // Shadow map generation
        Shader gBufferShader("shaders/gbuffer.vert", "shaders/gbuffer.frag");             // Deferred geometry pass
        Shader rcShader("shaders/fullscreen.vert", "shaders/rc_cascade.frag");            // Radiance cascades computation
        Shader rcResolveShader("shaders/fullscreen.vert", "shaders/rc_temporal_resolve.frag"); // GI temporal reprojection + variance clamp
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
        RadianceCascades rc(1280, 800, 5);                     // 6-cascade radiance cascade GI system (high quality)
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
        float ambientStrength = 0.08f;  // Ambient light intensity (GUI slider; applied when enabled)
        bool giEnabled = true;          // Toggle for global illumination (default on)
        bool ssaoEnabled = false;       // Toggle for screen space ambient occlusion (default off)
        bool ssrEnabled = false;        // Toggle for screen space reflections (default off)
        bool lightEnabled = true;       // Toggle for main light (default on)
        bool skyboxEnabled = true;      // Procedural sky behind the scene (open scenes like Sponza)
        float giStrengthScale = 1.0f;   // User multiplier on GI strength (lower lets material/normal detail show)
        float giCascadeInterval = 2.0f; // World length of cascade 0's interval (large: default scene is glTF Sponza)
        bool paused = false;            // Toggle for pause state
        float pausedTime = 0.0f;        // Time accumulator for pause system
        int antiAliasingMode = 2;       // AA mode: 0=none, 1=FXAA, 2=TAA (default TAA)
        int qualityLevel = 1;           // Quality level: 0=super low, 1=performance, 2=balanced, 3=high, 4=ultra (default: Performance for better FPS)
        bool cullingEnabled = false;    // Leave frustum culling off by default (avoid false negatives)

        // Sampling method state
        enum SamplingMethod {
            CUBE_FACE_SUBDIVISION = 0,
            LATLON_SUBDIVISION,
            GOLDEN_SPIRAL,
            KOGAN_SPIRAL,
            GOLDEN_HEMISPHERE,
            RANDOM_HEMISPHERE,
            UNIFORM_RANDOM_HEMISPHERE, // default
            UNIFORM_HEMISPHERE,
            SAMPLING_METHOD_COUNT
        };
        int samplingMethod = UNIFORM_RANDOM_HEMISPHERE;
        const char* samplingNames[] = {
            "Cube Face Subdivision",
            "Lat/Lon Subdivision",
            "Golden Spiral",
            "Kogan Spiral",
            "Golden Hemisphere",
            "Random Hemisphere",
            "Uniform Random Hemisphere",
            "Uniform Hemisphere"
        };

        // Culling statistics
        int totalEntities = 0;          // Total entities in scene
        int culledEntities = 0;         // Entities culled this frame
        int renderedEntities = 0;       // Entities rendered this frame

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
        
        // Performance monitoring for profiler output
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
                frameCount = 0;
                fpsTimer -= 1.0f;
            }
            profiler.endTimer("frame_setup");
            
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
                rc.resetTemporalAccumulation();
            }
            if (inputData.ssaoToggle.exchange(false)) {
                ssaoEnabled = !ssaoEnabled;
            }
            if (inputData.lightToggle.exchange(false)) {
                lightEnabled = !lightEnabled;
                // Reset temporal accumulation when light state changes
                rc.resetTemporalAccumulation();
            }
            if (inputData.ssrToggle.exchange(false)) {
                ssrEnabled = !ssrEnabled;
            }
            if (inputData.antiAliasingToggle.exchange(false)) {
                antiAliasingMode = (antiAliasingMode + 1) % 3; // Cycle: None -> FXAA -> TAA -> None
            }
            if (inputData.samplingToggle.exchange(false)) {
                samplingMethod = (samplingMethod + 1) % SAMPLING_METHOD_COUNT;
                rc.resetTemporalAccumulation();
            }
            if (inputData.cullingToggle.exchange(false)) {
                cullingEnabled = !cullingEnabled;
            }
            if (inputData.qualityToggle.exchange(false)) {
                qualityLevel = (qualityLevel + 1) % 5; // 5 quality levels: 0-4
                rc.resetTemporalAccumulation();
            }

            if (inputData.resetTemporal.exchange(false)) {
                rc.resetTemporalAccumulation();
            }
            if (inputData.pauseToggle.exchange(false)) {
                paused = !paused;
                g_cursorCaptured = !paused;
                if (paused) {
                    pausedTime = glfwGetTime();
                    glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            
            // Process camera movement (only when not paused).
            // NOTE: do NOT reset temporal accumulation on movement - the GI trace uses
            // motion-vector reprojection + a neighbourhood variance clamp, which keep the
            // accumulated result stable through camera motion. Resetting here would discard
            // all accumulated samples every frame and expose the raw, blotchy single-frame
            // estimate - the visible "quality drops while moving" symptom.
            if (!paused) {
                if (inputData.moveForward)  { scene.camera.processKeyboard(0, deltaTime); }
                if (inputData.moveBackward) { scene.camera.processKeyboard(1, deltaTime); }
                if (inputData.moveLeft)     { scene.camera.processKeyboard(2, deltaTime); }
                if (inputData.moveRight)    { scene.camera.processKeyboard(3, deltaTime); }
            }

            // Process light controls from async input (only when not paused)
            if (!paused) {
                // Apply light movement / property changes. As with the camera, we do NOT
                // reset temporal accumulation here: the variance clamp lets the accumulated
                // GI adapt to the new lighting over a few frames (a slight settle is far
                // preferable to the blotchy single-frame estimate a hard reset would show).
                for (const auto& entity : scene.entities) {
                    if (auto light = entity->getComponent<LightComponent>()) {
                        if (auto transform = entity->getComponent<TransformComponent>()) {
                            // Apply light movement from async input
                            if (inputData.lightMoveX != 0.0f || inputData.lightMoveZ != 0.0f || inputData.lightMoveY != 0.0f) {
                                transform->position.x += inputData.lightMoveX * deltaTime * 60.0f; // Scale by frame rate
                                transform->position.z += inputData.lightMoveZ * deltaTime * 60.0f;
                                transform->position.y += inputData.lightMoveY * deltaTime * 60.0f;
                            }

                            // Apply light property changes
                            if (inputData.lightIntensityDelta != 0.0f) {
                                light->intensity += inputData.lightIntensityDelta * deltaTime * 60.0f;
                                light->intensity = std::max(0.0f, light->intensity);
                            }

                            if (inputData.lightRadiusDelta != 0.0f) {
                                light->radius += inputData.lightRadiusDelta * deltaTime * 60.0f;
                                light->radius = std::max(0.5f, light->radius);
                            }
                        }
                    }
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

            // Update camera frustum for culling (only when culling is enabled)
            if (cullingEnabled) {
                scene.camera.updateFrustum(projection);
            }

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
                        // Apply same frustum culling to shadow pass for consistency
                        bool shouldRender = true;
                        if (cullingEnabled) {
                            glm::vec3 center = transform->getBoundingCenter();
                            float radius = transform->getBoundingRadius(15.0f); // generous radius to avoid over-cull
                            shouldRender = scene.camera.isSphereInFrustum(center, radius);
                        }
                        
                        if (shouldRender) {
                            shadowShader.setMat4("model", transform->getModelMatrix());
                            meshComp->mesh->Draw(shadowShader.ID);
                        }
                    }
                }
            }
            profiler.endTimer("shadow_render");
            profiler.endTimer("shadow_total");
            
            shadowTime = profiler.getLastTime("shadow_total");

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

            // Render all scene geometry to G-buffer with optional frustum culling
            profiler.beginTimer("gbuffer_render");
            totalEntities = static_cast<int>(scene.entities.size());
            culledEntities = 0;
            renderedEntities = 0;
            
            for (const auto& entity : scene.entities) {
                auto meshComp = entity->getComponent<MeshComponent>();
                auto transformComp = entity->getComponent<TransformComponent>();
                auto materialComp = entity->getComponent<MaterialComponent>();

                if (meshComp && transformComp && meshComp->mesh) {
                    // Frustum culling check (if enabled). Uses the MESH's bounding sphere
                    // transformed into world space - the entity transform alone is wrong for
                    // baked-transform geometry (e.g. glTF, where every entity sits at origin).
                    bool shouldRender = true;
                    if (cullingEnabled) {
                        glm::mat4 M = transformComp->getModelMatrix();
                        glm::vec3 center = glm::vec3(M * glm::vec4(meshComp->mesh->boundsCenter, 1.0f));
                        float maxScale = std::max({ glm::length(glm::vec3(M[0])),
                                                    glm::length(glm::vec3(M[1])),
                                                    glm::length(glm::vec3(M[2])) });
                        float radius = meshComp->mesh->boundsRadius * maxScale + 0.5f; // small margin

                        shouldRender = scene.camera.isSphereInFrustum(center, radius);
                        if (!shouldRender) {
                            culledEntities++;
                            continue; // Skip rendering this entity
                        }
                    }
                    
                    renderedEntities++;
                    
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
                } else {
                    // Count non-renderable entities as culled if they don't have required components
                    culledEntities++;
                }
            }
            profiler.endTimer("gbuffer_render");
            profiler.endTimer("gbuffer_total");
            
            gbufferTime = profiler.getLastTime("gbuffer_total");
            
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
                    case 4: activeCascades = 5; break; // Ultra (maximum quality)
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
                // Quality-dependent ray-march steps and angular budgets.
                // Per the radiance cascades penumbra criterion, nearAngular is cascade 0's
                // (small) sample count and farAngular is the cap that the far cascades grow
                // toward (doubling per level). near < far: the near field varies little
                // angularly, the far field a lot - and far cascades are cheap (low-res).
                int raySteps = 6;
                int nearAngular = 8;
                int farAngular = 48;
                switch (qualityLevel) {
                    case 0: raySteps = 3; nearAngular = 6;  farAngular = 24; break;  // Super Low
                    case 1: raySteps = 4; nearAngular = 8;  farAngular = 32; break;  // Performance
                    case 2: raySteps = 6; nearAngular = 8;  farAngular = 48; break;  // Balanced
                    case 3: raySteps = 8; nearAngular = 10; farAngular = 64; break;  // High
                    case 4: raySteps = 10; nearAngular = 12; farAngular = 96; break; // Ultra
                }
                rcShader.setInt("rayMarchSteps", raySteps);
                rcShader.setInt("samplingMethod", samplingMethod);
                // Scene-scaled cascade interval, then recompute band-limiting parameters.
                rc.setCascadeBaseInterval(giCascadeInterval);
                rc.setBandLimitingParameters(nearAngular, farAngular, 0.6f);
                profiler.endTimer("gi_setup");
                
                profiler.beginTimer("gi_compute");
                rc.compute(rcShader, rcResolveShader, view, projection, activeCascades);
                profiler.endTimer("gi_compute");

                // PASS 6: GI QUALITY-DEPENDENT BLUR
                // Apply blur based on quality level for optimal performance/quality balance
                profiler.beginTimer("gi_blur");
                {
                    switch (qualityLevel) {
                        case 0: // Super Low: minimal blur for performance
                            break;
                        case 1: // Performance: reduced blur (only blur first 2 cascades)
                            rc.blur(blurShader, activeCascades >= 2 ? 2 : activeCascades);
                            break;
                        case 2: // Balanced
                        case 3: // High
                        case 4: // Ultra
                            rc.blur(blurShader, activeCascades);
                            break;
                    }
                }
                profiler.endTimer("gi_blur");
            }
            profiler.endTimer("gi_total");
            
            giTime = profiler.getLastTime("gi_total");
            
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
            compositeShader.setMat4("invView", glm::inverse(view));
            compositeShader.setMat4("invProjection", glm::inverse(projection));
            compositeShader.setBool("enableSkybox", skyboxEnabled);
            compositeShader.setVec3("lightPos", lightPos);
            compositeShader.setVec3("lightColor", lightColor);
            compositeShader.setVec3("viewPos", scene.camera.position);
            compositeShader.setFloat("lightRadius", lightRadius);
            // CORRECTED GI strength: More cascades capture more light, so need LOWER multipliers for visual consistency
            // Ultra mode has additional enhancements (multi-bounce, better upsampling) so needs even lower strength
            float giStrength = 0.0f;
            if (giEnabled) {
                // Lowered ~0.6x vs before: the energy-correct cascade merge (transmittance
                // weighting instead of the old 0.4 loss factor) now propagates more
                // far-field radiance, so less composite gain is needed for the same look.
                switch (qualityLevel) {
                    case 0: giStrength = 0.52f; break; // Super Low (2C)
                    case 1: giStrength = 0.42f; break; // Performance (3C)
                    case 2: giStrength = 0.33f; break; // Balanced (4C)
                    case 3: giStrength = 0.27f; break; // High (5C) - reference
                    case 4: giStrength = 0.50f; break; // Ultra
                    default: giStrength = 0.33f; break; // Fallback to balanced
                }
            }
            compositeShader.setFloat("ssgiStrength", giStrength * giStrengthScale);
            compositeShader.setFloat("ambientStrength", ambientEnabled ? ambientStrength : 0.0f); // GUI-controlled ambient
            compositeShader.setFloat("ssaoStrength", (ssaoEnabled && qualityLevel > 0) ? 1.0f : 0.0f); // Conditional SSAO contribution

            compositeShader.setInt("activeCascades", activeCascades);

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
            
            // Bind the per-cascade GI textures for sampling.
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
            
            glEnable(GL_DEPTH_TEST);

            // PASS 8: SCREEN SPACE REFLECTIONS (Optional)
            if (ssrEnabled) {
                profiler.beginTimer("ssr_total");
                // Trace reflections (ssr.frag outputs premultiplied reflection.rgb, a=strength).
                rc.computeSSR(ssrShader, compositeTexture, view, projection, scene.camera.position);
                // Composite them OVER the scene: dst*(1-a) + src. Without this the SSR result
                // was computed into its own texture and never used (the reason "SSR didn't work").
                glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
                glViewport(0, 0, width, height);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                copyShader.use();
                copyShader.setInt("inputTexture", 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, rc.getSSRTexture());
                quad.render();
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
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

            // PASS 9: CONTROL PANEL (interactive ImGui widgets bound directly to state)
            profiler.beginTimer("ui_total");

            profiler.beginTimer("ui_setup");
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            profiler.endTimer("ui_setup");

            profiler.beginTimer("ui_render");
            const ImVec4 COL_ACCENT(0.4f, 0.85f, 1.0f, 1.0f);
            const ImVec4 COL_MUTED (0.6f, 0.6f, 0.6f, 1.0f);

            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(0.85f);
            if (ImGui::Begin("Vibe-GI", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                // --- Stats (read-only) ---
                ImGui::TextColored(COL_ACCENT, "Vibe-GI: Real-time Global Illumination");
                ImGui::TextColored(COL_MUTED, "Build %s (%s)", BUILD_NUMBER, BUILD_DATE);
                glm::vec3 camPos = scene.camera.position;
                ImGui::Text("FPS: %d   Camera: (%.1f, %.1f, %.1f)", fps, camPos.x, camPos.y, camPos.z);

                // --- Toggles (bound directly to render state) ---
                ImGui::SeparatorText("Rendering");
                if (ImGui::Checkbox("Global Illumination", &giEnabled)) rc.resetTemporalAccumulation();
                ImGui::Checkbox("SSAO", &ssaoEnabled); ImGui::SameLine();
                ImGui::Checkbox("SSR", &ssrEnabled);
                ImGui::Checkbox("Ambient", &ambientEnabled); ImGui::SameLine();
                if (ImGui::Checkbox("Main Light", &lightEnabled)) rc.resetTemporalAccumulation();
                ImGui::BeginDisabled(!ambientEnabled);
                ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f, "%.3f");
                ImGui::EndDisabled();
                ImGui::Checkbox("Frustum Culling", &cullingEnabled); ImGui::SameLine();
                ImGui::Checkbox("Skybox", &skyboxEnabled);
                ImGui::TextColored(COL_MUTED, "Culled: %d / %d", culledEntities, totalEntities);

                // --- Light controls (edit the primary light's ECS component directly) ---
                ImGui::SeparatorText("Light");
                {
                    LightComponent* primaryLight = nullptr;
                    for (const auto& e : scene.entities) {
                        if (auto lc = e->getComponent<LightComponent>()) { primaryLight = lc; break; }
                    }
                    if (primaryLight) {
                        ImGui::ColorEdit3("Color", &primaryLight->color[0], ImGuiColorEditFlags_NoInputs);
                        ImGui::SliderFloat("Intensity", &primaryLight->intensity, 0.0f, 300.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
                        ImGui::SliderFloat("Radius", &primaryLight->radius, 0.5f, 120.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
                        ImGui::TextColored(COL_MUTED, "Presets:"); ImGui::SameLine();
                        if (ImGui::SmallButton("White"))   primaryLight->color = glm::vec3(1.0f, 1.0f, 1.0f);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Day"))     primaryLight->color = glm::vec3(1.0f, 0.95f, 0.88f);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Evening")) primaryLight->color = glm::vec3(1.0f, 0.55f, 0.30f);
                    } else {
                        ImGui::TextColored(COL_MUTED, "No light in scene");
                    }
                }

                // --- Quality / sampling (combos) ---
                ImGui::SeparatorText("Quality");
                const char* qualityNames[] = { "Super Low (2C)", "Performance (3C)",
                                               "Balanced (4C)", "High (5C)", "Ultra (5C)" };
                if (ImGui::Combo("Quality", &qualityLevel, qualityNames, IM_ARRAYSIZE(qualityNames)))
                    rc.resetTemporalAccumulation();
                // Lower this if strong GI is washing out material/normal-map detail (flat look).
                ImGui::SliderFloat("GI Strength", &giStrengthScale, 0.0f, 2.0f, "%.2f");
                const char* aaNames[] = { "None", "FXAA", "TAA" };
                ImGui::Combo("Anti-Aliasing", &antiAliasingMode, aaNames, IM_ARRAYSIZE(aaNames));
                if (ImGui::Combo("Sampling", &samplingMethod, samplingNames, SAMPLING_METHOD_COUNT))
                    rc.resetTemporalAccumulation();
                // Cascade 0 interval length (world units). Small for the Cornell box, large
                // for building-scale scenes like glTF Sponza so GI reaches across the space.
                if (ImGui::SliderFloat("GI Cascade Interval", &giCascadeInterval, 0.02f, 8.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
                    rc.resetTemporalAccumulation();

                // --- Scene selector (reuses the existing async load path) ---
                ImGui::SeparatorText("Scene");
                const char* sceneNames[] = { "Cornell Box", "Teapot Lightbox", "Stone Floor",
                                             "Shadow Test", "Default Lightbox", "Sponza Overhead",
                                             "Sponza (glTF PBR)" };
                int sceneIdx = static_cast<int>(scene.currentScene);
                if (ImGui::Combo("Scene", &sceneIdx, sceneNames, IM_ARRAYSIZE(sceneNames)))
                    inputData.sceneSelection = sceneIdx; // handled at end of loop (loads + resets temporal)

                // --- Keyboard reference (collapsed by default) ---
                if (ImGui::CollapsingHeader("Keyboard Shortcuts")) {
                    ImGui::TextColored(COL_MUTED,
                        "WASD: move   Mouse: look   Space: pause/cursor\n"
                        "1-6: scene   G/T/F: GI/SSAO/SSR   M: ambient\n"
                        "Z: quality   C: AA   J: culling   ?: sampling\n"
                        "Arrows: move light   K/L: height   O/P: intensity   I/U: radius\n"
                        "X: perf metrics   R: reset accumulation");
                }
            }
            ImGui::End();
            profiler.endTimer("ui_render");

            profiler.beginTimer("ui_cleanup");
            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            profiler.endTimer("ui_cleanup");
            profiler.endTimer("ui_total");
            
            float uiTime = profiler.getLastTime("ui_total");

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
            
            // Log detailed performance breakdown every 60 frames
            if (inputData.showPerformance.exchange(false)) { // Only log if requested
                profiler.logDetailedStats();
            }
            
            // Handle scene selection
            int selectedScene = inputData.sceneSelection.exchange(-1);
            if (selectedScene >= 0 && selectedScene <= 6) {
                scene.loadScene(static_cast<Scene::SceneType>(selectedScene));
                // Big building-scale scenes need a much larger cascade interval than the
                // Cornell-box default so GI reaches across the space.
                giCascadeInterval = (selectedScene == Scene::GLTF_SPONZA) ? 2.0f : 0.125f;
                // Reset temporal accumulation when scene changes
                rc.resetTemporalAccumulation();
            }
        }

        // Clean up multithreading resources
        inputThreadRunning = false;
        inputThread.join();

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

    // In mouse/GUI mode (cursor not captured) the pointer drives the UI, not the camera.
    // We still updated lastX/lastY above so re-entering mouselook doesn't snap the view.
    if (!g_cursorCaptured.load()) {
        return;
    }

    // Apply mouse movement to camera
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (camera) {
        camera->processMouse(xoffset, yoffset);
    }
}

// Re-assert the desired cursor mode whenever the window (re)gains focus. This is what
// makes mouselook capture engage at startup and after alt-tab on Wayland, where
// GLFW_CURSOR_DISABLED is a no-op until the window actually holds pointer focus.
void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        glfwSetInputMode(window, GLFW_CURSOR,
                         g_cursorCaptured.load() ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}
