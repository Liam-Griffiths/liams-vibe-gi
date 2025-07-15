
# Basic 3D Game Engine in C++

This is a simple modular game engine built in C++ demonstrating basic lighting and a movable camera. It uses an Entity-Component-System (ECS) architecture for modularity and is well-commented for learning purposes.

## Features
- Window management with GLFW
- Basic camera movement (WASD keys + mouse)
- Simple lighting demo with point light
- Mesh rendering with shaders
- ECS architecture

## Prerequisites
- C++17 compatible compiler (e.g., g++ or clang++)
- CMake 3.10+
- GLFW library
- GLM library
- OpenGL (included in macOS)

On macOS, install dependencies via Homebrew:
```
brew install cmake glfw glm
```

## Build Instructions
1. Clone the repository (or navigate to the project directory).
2. Create a build directory:
   ```
   mkdir build
   cd build
   ```
3. Run CMake:
   ```
   cmake ..
   ```
4. Build the project:
   ```
   make
   ```

## Running the Demo
From the build directory:
```
./3dEngine
```

Use WASD to move, mouse to look around. ESC to quit.

## Project Structure
- `src/`: Source files
- `include/`: Header files
- `shaders/`: Shader files
- `CMakeLists.txt`: Build configuration

## Learning Notes
The code is heavily commented. Start with `src/main.cpp` to see the engine loop. Check `include/` for component definitions. 