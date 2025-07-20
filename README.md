
# Vibe-GI: Advanced Real-Time Global Illumination Renderer

A sophisticated real-time global illumination renderer implementing cutting-edge graphics techniques in C++. This project demonstrates advanced rendering algorithms including radiance cascades, screen-space effects, and temporal filtering, all wrapped in a clean, educational codebase.

## Screenshots

<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 10 57" src="https://github.com/user-attachments/assets/79c82bbc-cbbc-46fb-8631-318870a86068" />
<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 11 43" src="https://github.com/user-attachments/assets/6a46fc1e-23bc-4de7-a3a5-743539cea269" />

## Core Features

### Advanced Global Illumination
- **Radiance Cascades**: State-of-the-art real-time GI with multi-bounce indirect lighting
- **Adaptive Quality System**: 5 performance levels (2-6 cascades) with automatic brightness balancing
- **Temporal Stability**: Advanced temporal filtering with motion vector reprojection
- **Multi-Scale Representation**: Hierarchical lighting capture from near-field contact shadows to far-field environment lighting

### Modern Rendering Pipeline
- **Deferred Rendering**: G-buffer based pipeline for efficient lighting calculations
- **PBR Materials**: Physically Based Rendering with albedo, normal, roughness, and metallic properties
- **Screen-Space Effects**: 
  - **SSAO**: Contact shadows with bilateral noise reduction
  - **SSR**: High-quality reflections with adaptive raymarching
- **Anti-Aliasing**: Dual TAA (Temporal) and FXAA (Spatial) systems
- **Dynamic Lighting**: Real-time light positioning, intensity, and radius controls

### Performance & Quality
- **Real-Time Performance**: 20-60+ FPS depending on quality settings
- **Scalable Quality**: From mobile-friendly 2-cascade setup to high-end 6-cascade ultra quality
- **Performance Profiling**: Comprehensive frame timing breakdown with 25+ metrics
- **Optimized Memory Usage**: 16-bit precision cascades with efficient temporal accumulation

### Scene & Content
- **Cornell Box Demo**: Classic computer graphics test scene with proper color bleeding
- **PBR Materials**: Stone textures with full material property maps
- **Dynamic Objects**: Animated teapot with rotation behavior
- **Emissive Surfaces**: Light-emitting geometry for complex lighting scenarios
- **Entity-Component-System**: Clean, modular architecture for scene management

## Technical Implementation

### Radiance Cascades Algorithm
The core GI system uses a hierarchical approach to capture lighting at multiple spatial scales:

```
Cascade 0 (Near): Full resolution    - Contact shadows, local detail
Cascade 1 (Mid):  3/4 resolution    - Medium-distance lighting  
Cascade 2 (Far):  1/2 resolution    - Broad environmental lighting
Higher Cascades:  Progressive LOD    - Distant lighting contribution
```

Each cascade uses adaptive angular sampling and band-limited capture to ensure proper frequency representation across distance ranges.

### Quality Level Breakdown
- **Super Low (2 cascades)**: 60+ FPS, minimal GI for integrated graphics
- **Performance (3 cascades)**: 45-60 FPS, good balance for gaming
- **Balanced (4 cascades)**: 35-45 FPS, enhanced quality for mid-range GPUs
- **High (5 cascades)**: 25-35 FPS, excellent fidelity for high-end systems  
- **Ultra (6 cascades)**: 20-30 FPS, maximum quality for benchmarking

### Shader Pipeline Overview
```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│   G-Buffer  │ -> │   Radiance   │ -> │    SSAO     │
│ Generation  │    │  Cascades    │    │ Computation │
└─────────────┘    └──────────────┘    └─────────────┘
        │                  │                   │
        v                  v                   v
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│    SSR      │ -> │   Lighting   │ -> │     TAA     │
│Reflections  │    │  Composite   │    │   / FXAA    │
└─────────────┘    └──────────────┘    └─────────────┘
```

## Controls

### Camera & Navigation
- **WASD**: Camera movement (first-person controls)
- **Mouse**: Look around (captured mouse mode)
- **ESC**: Exit application

### Rendering Quality & Effects
- **Z**: Cycle quality levels (Super Low → Performance → Balanced → High → Ultra)
- **G**: Toggle global illumination on/off
- **T**: Toggle SSAO (Screen Space Ambient Occlusion)
- **F**: Toggle SSR (Screen Space Reflections)
- **C**: Cycle anti-aliasing modes (None → FXAA → TAA)
- **M**: Toggle ambient lighting contribution

### Dynamic Lighting Controls
- **Arrow Keys**: Move directional light position (XZ plane)
- **K/L**: Adjust light height (Y axis)
- **O/P**: Control light intensity (brightness)
- **I/U**: Adjust light radius (attenuation falloff)
- **V**: Toggle main light on/off

### Performance & Debugging
- **X**: Show detailed performance breakdown (25+ timing metrics)
- **R**: Reset temporal accumulation (clears GI history)
- **Space**: Pause/unpause rendering (enables cursor for UI interaction)

## Build Instructions

### Prerequisites

#### macOS
```bash
brew install cmake glfw glm freetype
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt install build-essential cmake libglfw3-dev libglm-dev libfreetype6-dev libgl1-mesa-dev
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install cmake gcc-c++ glfw-devel glm-devel freetype-devel mesa-libGL-devel
```

#### Windows (vcpkg)
```cmd
vcpkg install glfw3 glm freetype
```

### Quick Start

1. **Clone with submodules** (ImGui included):
   ```bash
   git clone --recursive <repository-url>
   cd vibe-gi
   ```
   
2. **Build and run** (Linux/macOS):
   ```bash
   # Development build
   ./build.sh

   # Optimized release build
   ./build.sh release

   # Build and immediately run
   ./build.sh release run
   ```

3. **Manual build** (all platforms):
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)  # Linux
   make -j$(sysctl -n hw.ncpu)  # macOS
   ```

### Windows Build
   ```cmd
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

## Project Structure

```
vibe-gi/
├── src/                    # Core implementation
│   ├── main.cpp           # Main render loop & application entry
│   ├── Camera.cpp         # First-person camera system
│   ├── Scene.cpp          # ECS scene management
│   ├── RadianceCascades.cpp # Global illumination implementation
│   └── [other components] # Materials, meshes, etc.
├── include/               # Header files
│   ├── RadianceCascades.h # Comprehensive GI system interface
│   ├── Camera.h           # Camera controls and matrices
│   └── [ECS components]   # Transform, Mesh, Material, Light components
├── shaders/               # GLSL shader programs
│   ├── gbuffer.*          # Deferred rendering geometry pass
│   ├── rc_cascade.frag    # Radiance cascades computation
│   ├── final_composite.frag # Lighting combination and tone mapping
│   ├── ssao.*             # Screen-space ambient occlusion
│   ├── ssr.frag           # Screen-space reflections
│   ├── taa.frag           # Temporal anti-aliasing
│   └── [other effects]    # Blur, FXAA, shadow mapping
├── textures/              # PBR material assets
├── models/                # OBJ mesh files (teapot, etc.)
├── scripts/               # Behavior components
├── third_party/imgui/     # Immediate mode GUI (submodule)
├── build.sh              # Automated build script
├── clean.sh              # Project cleanup
└── CMakeLists.txt        # Build configuration
```

## Technical Deep Dive

### Radiance Cascades Implementation
The GI system maintains multiple spatial representations of lighting:

**Near-Field Cascades (0-1)**: High resolution for local lighting effects
- Contact shadows and surface-to-surface light transfer
- Fine geometric detail preservation
- High angular sampling for accurate normal-dependent lighting

**Far-Field Cascades (2+)**: Lower resolution for environmental lighting
- Broad illumination patterns and environment lighting
- Reduced angular sampling for performance
- Progressive level-of-detail with distance

### Performance Optimization Techniques
- **Temporal Accumulation**: Exponential moving average reduces per-frame computation
- **Adaptive Sampling**: Angular resolution scales with cascade distance range
- **Band-Limited Capture**: Prevents aliasing and ensures stable convergence
- **Separable Filtering**: Bilateral blur reduces noise while preserving edges
- **Dynamic Quality**: Runtime cascade count adjustment for target frame rates

### Screen-Space Effects
- **SSAO**: Hemisphere sampling with random rotation for contact shadows
- **SSR**: Adaptive raymarching with binary search refinement
- **TAA**: Motion vector reprojection with YCoCg color space variance clamping
- **FXAA**: Subpixel edge detection with adaptive sampling

## Learning & Educational Value

This project serves as a comprehensive learning resource for modern real-time rendering:

### Graphics Programming Concepts
- **Global Illumination**: Multi-bounce indirect lighting simulation
- **Temporal Methods**: History buffers and motion vector reprojection
- **Screen-Space Techniques**: SSAO, SSR, and anti-aliasing algorithms
- **Performance Optimization**: Quality scaling and frame timing analysis

### Software Architecture
- **Entity-Component-System**: Modern game engine architecture patterns
- **Resource Management**: OpenGL buffer and texture lifecycle management
- **Threading**: Asynchronous input processing for responsive controls
- **Profiling**: Comprehensive performance measurement and optimization

### Code Quality
- **Extensive Documentation**: 500+ lines of technical comments
- **Clean Architecture**: Modular design with clear separation of concerns
- **Error Handling**: Comprehensive OpenGL error checking and resource cleanup
- **Educational Comments**: Algorithm explanations and implementation notes

## System Requirements

### Minimum Requirements
- **OpenGL**: 3.3+ compatible graphics card
- **CPU**: Dual-core processor
- **RAM**: 4GB system memory
- **Resolution**: 720p display

### Recommended Specifications
- **GPU**: Dedicated graphics card with 2GB+ VRAM
- **CPU**: Quad-core processor (Intel i5/AMD Ryzen 5 or better)
- **RAM**: 8GB+ system memory
- **Resolution**: 1080p+ display for optimal visual experience

### Performance Expectations
- **Integrated Graphics**: 30-45 FPS on Super Low/Performance settings
- **Mid-Range GPU**: 45-60 FPS on Balanced/High settings
- **High-End GPU**: 60+ FPS on Ultra settings with all effects enabled

## Contributing & Experimentation

The codebase is designed for experimentation and learning:

- **Modular Shaders**: Easy to modify and experiment with rendering techniques
- **Configurable Parameters**: Extensive runtime configuration for algorithm tuning
- **Clear Documentation**: Well-commented code explains implementation details
- **Educational Structure**: Progressive complexity from basic to advanced techniques

Feel free to experiment with:
- **New GI Techniques**: Alternative global illumination algorithms
- **Advanced Materials**: Extended PBR models and material properties
- **Additional Effects**: Volumetrics, subsurface scattering, etc.
- **Performance Optimizations**: Compute shaders, advanced culling, etc.

## Philosophy

*"Advanced graphics programming with clarity and educational value."*

Vibe-GI demonstrates that sophisticated rendering techniques can be implemented with clean, understandable code. The project balances technical advancement with educational accessibility, making modern graphics programming concepts approachable for developers at all levels.

## License

This project is open-source and available for educational and research purposes. See individual component licenses for specific terms. 
