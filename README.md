
# Vibe-GI: A Vibed Out Global Illumination Renderer

A casually vibe-coded yet sophisticated global illumination renderer built in C++. This project implements advanced real-time rendering techniques with a chill, experimental approach to graphics programming. That being, having no idea what i'm doing.

## Screenshots

*Screenshots coming soon!

## Features

### Advanced Teapot Rendering
- **Industry-Leading Teapot Technology**: Because every graphics engine needs to render the Utah teapot with maximum sophistication
- **Photorealistic Spout Illumination**: Watch light bounce off that iconic teapot handle like never before

### Global Illumination & Lighting
- **Radiance Cascades**: Advanced global illumination technique for realistic indirect lighting
- **Screen Space Ambient Occlusion (SSAO)**: Real-time ambient occlusion for enhanced depth perception
- **Shadow Mapping**: High-quality directional light shadows
- **Physically Based Rendering (PBR)**: Material system with albedo, normal, roughness, and AO maps

### Rendering Pipeline
- **Deferred Rendering**: G-buffer based rendering for efficient lighting calculations
- **Temporal Anti-Aliasing (TAA)**: Motion-based temporal upsampling for smooth edges
- **Multi-pass Post-Processing**: Blur, copy, and composite passes for polished output
- **Text Rendering**: FreeType-based text overlay system

### Architecture
- **Entity-Component-System (ECS)**: Clean, modular architecture for scene management
- **Modern OpenGL**: Shader-based rendering pipeline
- **Real-time Camera**: WASD + mouse controls for scene exploration

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

### Linux/macOS
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd vibe-gi
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
1. Clone the repository:
   ```cmd
   git clone <repository-url>
   cd vibe-gi
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
   git clone <repository-url>
   cd vibe-gi
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles"
   mingw32-make -j$(nproc)
   ```

## Running the Renderer

### Linux/macOS
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
- **WASD**: Camera movement
- **Mouse**: Look around
- **ESC**: Exit

## Project Structure
```
vibe-gi/
├── src/           # Core implementation files
├── include/       # Header files with component definitions
├── shaders/       # GLSL shader programs
├── textures/      # PBR texture assets
├── models/        # 3D model files
└── fonts/         # Text rendering fonts
```

## Technical Implementation

### Rendering Techniques
- **Radiance Cascades**: Implements the cutting-edge radiance cascades GI algorithm
- **Deferred Shading**: G-buffer stores albedo, normal, depth, and material properties
- **TAA Integration**: Temporal accumulation for high-quality anti-aliasing
- **Multi-bounce Lighting**: Realistic light bouncing for natural illumination

### Shader Pipeline
- `gbuffer.*`: Geometry buffer generation
- `rc_cascade.frag`: Radiance cascades computation
- `lighting.*`: Deferred lighting calculations
- `ssao.*`: Screen-space ambient occlusion
- `taa.frag`: Temporal anti-aliasing
- `final_composite.frag`: Final image composition

## Learning & Exploration
This project serves as both a functional renderer and a learning resource. The codebase is extensively commented to explain rendering concepts, making it perfect for graphics programming education and experimentation.

Dive into `src/main.cpp` for the main render loop, explore the component system in `include/`, and check out the shader implementations for the nitty-gritty graphics details.

## Vibe
Built with a relaxed approach to high-performance graphics programming. Sometimes you just gotta vibe with the photons. ✨ 