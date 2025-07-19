
# Vibe-GI: A Vibed Out Global Illumination Renderer

A casually vibe-coded yet sophisticated global illumination renderer built in C++. This project implements advanced real-time rendering techniques with a chill, experimental approach to graphics programming. That being, having no idea what i'm doing.

## Screenshots

<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 10 57" src="https://github.com/user-attachments/assets/79c82bbc-cbbc-46fb-8631-318870a86068" />
<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 11 43" src="https://github.com/user-attachments/assets/6a46fc1e-23bc-4de7-a3a5-743539cea269" />


## Features

### Advanced Teapot Rendering
- **Industry-Leading Teapot Technology**: Because every graphics engine needs to render the Utah teapot with maximum sophistication
- **Photorealistic Spout Illumination**: Watch light bounce off that iconic teapot handle like never before

### Global Illumination & Lighting
- **Radiance Cascades**: Advanced global illumination technique for realistic indirect lighting
- **5 Quality Levels**: Super Low (2 cascades) to Ultra (6 cascades) with automatic quality balancing
- **Screen Space Ambient Occlusion (SSAO)**: Real-time ambient occlusion for enhanced depth perception
- **Shadow Mapping**: High-quality directional light shadows with configurable parameters
- **Physically Based Rendering (PBR)**: Material system with albedo, normal, roughness, and AO maps
- **Dynamic Lighting**: Interactive light positioning, intensity, and radius controls

### Advanced Visual Effects
- **Screen Space Reflections (SSR)**: 64-step adaptive raymarching with binary search refinement
- **Dual Anti-Aliasing System**: 
  - **TAA (Temporal Anti-Aliasing)**: Motion vector-based temporal accumulation with YCoCg color space
  - **FXAA (Fast Approximate Anti-Aliasing)**: Edge detection with adaptive sampling
- **Enhanced Temporal Filtering**: Multi-bounce GI with improved cascade blending
- **Material-Aware Reflections**: Surface roughness and fresnel effects for realistic reflections

### Rendering Pipeline
- **Deferred Rendering**: G-buffer based rendering for efficient lighting calculations
- **Multi-pass Post-Processing**: Blur, copy, and composite passes for polished output
- **Real-time Performance Monitoring**: Detailed frame timing breakdown with 25+ metrics
- **Interactive UI Overlay**: Real-time status display and controls documentation

### Architecture
- **Entity-Component-System (ECS)**: Clean, modular architecture for scene management
- **Modern OpenGL**: Shader-based rendering pipeline
- **Real-time Camera**: WASD + mouse controls for scene exploration

## Dependencies

This project uses git submodules to manage external dependencies:
- **ImGui**: Included as a submodule at `third_party/imgui` for immediate mode GUI
- **System libraries**: GLFW, GLM, FreeType (installed via package manager)

When cloning, use `--recursive` flag or run `git submodule update --init --recursive` to fetch all dependencies.

## Prerequisites

### All Platforms
- C++17 compatible compiler (g++, clang++, MSVC)
- CMake 3.10+
- GLFW library
- GLM library  
- FreeType library
- OpenGL 3.3+

### Platform-Specific Setup

#### macOS
Install dependencies via Homebrew:
```bash
brew install cmake glfw glm freetype
```

#### Linux (Ubuntu/Debian)
Install dependencies via apt:
```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglm-dev libfreetype6-dev libgl1-mesa-dev
```

#### Linux (Fedora/RHEL)
Install dependencies via dnf/yum:
```bash
sudo dnf install cmake gcc-c++ glfw-devel glm-devel freetype-devel mesa-libGL-devel
# or for older systems:
# sudo yum install cmake gcc-c++ glfw-devel glm-devel freetype-devel mesa-libGL-devel
```

#### Linux (Arch)
Install dependencies via pacman:
```bash
sudo pacman -S cmake gcc glfw glm freetype2 mesa
```

#### Windows (Visual Studio)
1. Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with C++ development tools
2. Install [vcpkg](https://github.com/Microsoft/vcpkg) for package management:
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```
3. Install dependencies:
   ```cmd
   .\vcpkg install glfw3 glm freetype
   ```

#### Windows (MSYS2/MinGW)
1. Install [MSYS2](https://www.msys2.org/)
2. In MSYS2 terminal:
   ```bash
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-glm mingw-w64-x86_64-freetype
   ```

## Build Instructions

### Quick Start (Recommended)

The easiest way to build the project is using the provided build scripts:

1. Clone the repository with submodules:
   ```bash
   git clone --recursive <repository-url>
   cd vibe-gi
   ```
   
   **Note**: If you've already cloned without `--recursive`, initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

2. Build and run (Linux/macOS):
   ```bash
   # Build in debug mode (default)
   ./build.sh

   # Build in release mode for better performance
   ./build.sh release

   # Clean and build
   ./build.sh clean debug

   # Build and immediately run
   ./build.sh release run

   # See all options
   ./build.sh help
   ```

3. Clean the project:
   ```bash
   ./clean.sh
   ```

### Manual Build (Alternative Method)

If you prefer to build manually or need custom configuration:

#### Linux/macOS
1. Clone the repository with submodules:
   ```bash
   git clone --recursive <repository-url>
   cd vibe-gi
   ```
   
   **Note**: If you've already cloned without `--recursive`, initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

2. Create and enter build directory:
   ```bash
   mkdir build && cd build
   ```

3. Configure with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   make -j$(nproc)  # Linux
   make -j$(sysctl -n hw.ncpu)  # macOS
   ```

### Windows (Visual Studio)
1. Clone the repository with submodules:
   ```cmd
   git clone --recursive <repository-url>
   cd vibe-gi
   ```
   
   **Note**: If you've already cloned without `--recursive`, initialize submodules:
   ```cmd
   git submodule update --init --recursive
   ```

2. Create build directory:
   ```cmd
   mkdir build && cd build
   ```

3. Configure with CMake (assuming vcpkg is set up):
   ```cmd
   cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

4. Build the project:
   ```cmd
   cmake --build . --config Release
   ```

### Windows (MSYS2/MinGW)
1. In MSYS2 MinGW64 terminal:
   ```bash
   git clone --recursive <repository-url>
   cd vibe-gi
   
   # If already cloned without --recursive:
   # git submodule update --init --recursive
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles"
   mingw32-make -j$(nproc)
   ```

## Running the Renderer

### Quick Start
The easiest way to run the renderer is using the build script:
```bash
# Build and run in one command
./build.sh release run
```

### Manual Execution
#### Linux/macOS
From the build directory:
```bash
./vibe-gi
```

### Windows
From the build directory:
```cmd
# Visual Studio build
Release\vibe-gi.exe

# MinGW build  
vibe-gi.exe
```

### Controls

#### Camera & Navigation
- **WASD**: Camera movement
- **Mouse**: Look around (first-person view)
- **ESC**: Exit application

#### Quality & Rendering
- **Z**: Cycle quality levels (Super Low → Performance → Balanced → High → Ultra)
- **M**: Toggle ambient lighting on/off
- **G**: Toggle global illumination on/off  
- **T**: Toggle SSAO (Screen Space Ambient Occlusion)
- **F**: Toggle SSR (Screen Space Reflections)
- **C**: Cycle anti-aliasing modes (None → FXAA → TAA → None)

#### Lighting Controls
- **Arrow Keys**: Move directional light position
- **K/L**: Adjust light height (up/down)
- **O/P**: Control light intensity (decrease/increase)
- **I/U**: Adjust light radius (decrease/increase)

#### Performance
- **X**: Show performance breakdown (detailed frame timing)

## Project Structure
```
vibe-gi/
├── src/           # Core implementation files
├── include/       # Header files with component definitions
├── shaders/       # GLSL shader programs
├── textures/      # PBR texture assets
├── models/        # 3D model files
├── build.sh       # Automated build script (Linux/macOS)
├── clean.sh       # Project cleanup script
├── CMakeLists.txt # CMake configuration
└── third_party/   # External dependencies (ImGui)
```

## Technical Implementation

### Rendering Techniques
- **Radiance Cascades**: Implements the cutting-edge radiance cascades GI algorithm with 2-6 cascades
- **Adaptive Quality System**: Dynamic cascade count with automatic brightness balancing
- **Deferred Shading**: G-buffer stores position, normal, albedo, and material properties
- **Advanced Temporal Filtering**: Motion vector-based reprojection with variance clamping
- **Multi-bounce Lighting**: Realistic light bouncing with temporal accumulation
- **Depth-Aware Reflections**: SSR with proper intersection testing and material awareness

### Quality Level Breakdown
- **Super Low (2 cascades)**: ~60+ FPS, basic GI with minimal temporal filtering
- **Performance (3 cascades)**: ~45-60 FPS, balanced quality and performance
- **Balanced (4 cascades)**: ~35-45 FPS, enhanced GI with better temporal stability
- **High (5 cascades)**: ~25-35 FPS, high-quality GI with advanced filtering
- **Ultra (6 cascades)**: ~20-30 FPS, maximum quality with multi-bounce effects

### Shader Pipeline
- `gbuffer.*`: Geometry buffer generation with motion vectors
- `rc_cascade.frag`: Radiance cascades computation with adaptive sampling
- `lighting.*`: Deferred lighting calculations with PBR materials
- `ssao.*`: Screen-space ambient occlusion with bilateral blur
- `ssr.frag`: Screen-space reflections with adaptive raymarching
- `taa.frag`: Temporal anti-aliasing with YCoCg color space
- `fxaa.frag`: Fast approximate anti-aliasing with edge detection
- `final_composite.frag`: Final image composition with tone mapping

## Performance & System Requirements

### Recommended Hardware
- **GPU**: OpenGL 3.3+ compatible graphics card
- **CPU**: Multi-core processor (4+ cores recommended)
- **RAM**: 4GB minimum, 8GB+ recommended
- **Display**: 1080p+ resolution for optimal visual experience

### Performance Characteristics
- **Real-time Performance**: 20-60+ FPS depending on quality level
- **Scalable Quality**: 5 distinct quality presets for different hardware
- **Adaptive Features**: Toggle individual effects based on performance needs
- **Memory Efficient**: Optimized cascade storage and temporal accumulation

### Platform Support
- **macOS**: Native support with Metal backend compatibility
- **Linux**: Full OpenGL support with various distributions
- **Windows**: DirectX/OpenGL compatibility with Visual Studio and MinGW builds

## Learning & Exploration
This project serves as both a functional renderer and a learning resource. The codebase is extensively commented to explain rendering concepts, making it perfect for graphics programming education and experimentation.

**Key Learning Areas:**
- **Global Illumination**: Radiance cascades implementation and theory
- **Real-time Reflections**: Screen-space techniques and optimizations  
- **Temporal Methods**: Anti-aliasing and accumulation strategies
- **Performance Optimization**: Quality scaling and frame timing analysis

Dive into `src/main.cpp` for the main render loop, explore the component system in `include/`, and check out the shader implementations for the nitty-gritty graphics details.

## Vibe
Built with a relaxed approach to high-performance graphics programming. Sometimes you just gotta vibe with the photons. Features industry-leading teapot technology that puts other renderers to shame. 🫖✨

*"Why render boring scenes when you can achieve photorealistic spout illumination?"* - The Vibe-GI Philosophy 
