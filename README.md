
# Vibe-GI: Experimental Radiance Cascades Implementation [WIP]

**⚠️ EXPERIMENTAL VIBE CODING PROJECT ⚠️**

An experimental **"vibe coding"** implementation of the **Radiance Cascades** global illumination technique described in the paper:

> **"Radiance Cascades: A Novel Approach to Calculating Global Illumination [WIP]"**  
> *by Alexander Sannikov*

## What is "Vibe Coding"?

This project is an experiment in **intuitive, exploratory programming** - implementing complex graphics algorithms through experimentation and iteration rather than strict engineering practices. Features are added based on "feel" and visual results rather than comprehensive planning.

**This means:**
- Code quality varies wildly
- Features implemented based on curiosity rather than requirements
- Bugs are features until proven otherwise
- Documentation written after the fact (or not at all)
- Performance optimizations are suggestions, not requirements

This is a research prototype implementing concepts from Sannikov's paper. **Many features are incomplete, broken, or experimental.** This is not production-ready software.

Captured on a Macbook Pro M1:

https://github.com/user-attachments/assets/a51b0a0b-0d9b-4396-a30d-41c1cc8b7b08

## ⚠️ Current Status & Warnings

### What's Working (Somewhat)
- ✅ Basic radiance cascades implementation (3-5 cascades)
- ✅ Cornell box scene with proper lighting
- ✅ Scene switching (1-6 keys) - though not all scenes work
- ✅ Real-time camera controls
- ✅ Multiple quality levels
- ✅ Basic performance monitoring

### What's Broken/Incomplete
- ❌ **SSAO shader compilation errors** (normal variable issues)
- ❌ **Sponza scene not working** (loads but doesn't render properly)
- ❌ **Frustum culling system** (too aggressive, disabled by default)
- ❌ Temporal accumulation stability issues
- ❌ SSR (Screen Space Reflections) - partially implemented
- ❌ TAA (Temporal Anti-Aliasing) - needs work
- ❌ PBR material system - inconsistent 
- ❌ Multi-light support - limited
- ❌ Memory leaks and resource management
- ❌ Cross-platform compatibility issues
- ❌ Performance optimization needed
- ❌ Shader errors on some systems

### Known Issues
- Shader compilation failures on certain configurations
- **Sponza scene broken** - loads model but doesn't render/display properly
- Performance drops in complex scenes
- Culling system too aggressive (disabled by default)
- Temporal filtering artifacts
- Memory usage not optimized
- Threading issues with input processing
- Scene switching inconsistent - some scenes don't work

## Original Paper Implementation

This implementation attempts to follow the concepts outlined in Alexander Sannikov's **"Radiance Cascades"** paper. The core ideas being explored:

- **Multi-scale radiance representation** across cascaded distance ranges
- **Hierarchical light transport** from near-field to far-field
- **Temporal accumulation** for stable convergence
- **Angular and spatial sampling** strategies

**Note**: This is an educational/research implementation and may not accurately reflect all aspects of the original paper. Many liberties have been taken, and numerous bugs exist.

## Screenshots

<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 10 57" src="https://github.com/user-attachments/assets/79c82bbc-cbbc-46fb-8631-318870a86068" />
<img width="1280" height="798" alt="Screenshot 2025-07-17 at 21 11 43" src="https://github.com/user-attachments/assets/6a46fc1e-23bc-4de7-a3a5-743539cea269" />

*Note: Screenshots may not reflect current state due to ongoing development.*

## Controls

### Scene Navigation
- **WASD**: Camera movement
- **Mouse**: Look around  
- **1-6**: Switch between scenes:
  - 1: Cornell Box (default - working)
  - 2: Teapot Lightbox (working)
  - 3: Stone Floor PBR (partial)
  - 4: Shadow Test (experimental)
  - 5: Default Lightbox (working)
  - 6: Sponza Overhead (**broken**)
- **ESC**: Exit

### Rendering Settings
- **Z**: Cycle quality levels (2-5 cascades)
- **G**: Toggle global illumination
- **T**: Toggle SSAO (currently broken)
- **F**: Toggle SSR (experimental)
- **C**: Cycle anti-aliasing modes
- **J**: Toggle frustum culling
- **M**: Toggle ambient lighting
- **V**: Toggle main light

### Light Controls
- **Arrow Keys**: Move light position
- **K/L**: Light height
- **O/P**: Light intensity  
- **I/U**: Light radius

### Debug
- **X**: Show performance metrics
- **R**: Reset temporal accumulation
- **Space**: Pause/unpause

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

**Expected issues**: Shader compilation errors, performance problems, visual artifacts, Sponza scene not working.

## Project Structure
```
vibe-gi/
├── src/                   # Core implementation (incomplete)
│   ├── main.cpp          # Main loop with threading issues
│   ├── RadianceCascades.cpp # GI implementation (experimental)
│   ├── Scene.cpp         # Scene management (basic)
│   └── [other files]     # Various states of completion
├── shaders/              # GLSL shaders (some broken)
│   ├── rc_cascade.frag   # Core GI shader
│   ├── ssao.frag         # Broken SSAO shader
│   └── [others]          # Mixed working/broken state
├── include/              # Headers
├── models/               # Test models (teapot, sponza, etc.)
└── textures/             # Basic PBR textures
```

## Research & Learning Value

This project serves as:
- **Educational exploration** of radiance cascades concepts
- **Reference implementation** for the Sannikov paper
- **Testbed for GI techniques** (work in progress)
- **Graphics programming learning** (with many bugs to fix)
- **Experiment in "vibe-based" development** - intuitive programming approach

### Vibe Coding Philosophy
- **Code by feel**: Implement what seems right, optimize later (or never)
- **Visual-first development**: If it looks good, it probably is good
- **Embrace the jank**: Bugs often reveal interesting behaviors
- **Documentation is optional**: Code should speak for itself (even if it doesn't)
- **Performance is negotiable**: 15 FPS is cinematic, right?

### Limitations for Learning
- Code quality varies significantly (by design)
- Documentation is incomplete (intentionally minimal)
- Many best practices violated (features, not bugs)
- Performance not optimized (character building)
- Cross-platform support lacking (works on my machine™)

## Technical Implementation Notes

### Radiance Cascades (Experimental)
The implementation attempts to follow the paper's concepts:

```
Cascade 0: Finest detail, highest resolution
Cascade 1: Medium detail, 3/4 resolution  
Cascade 2: Broader lighting, 1/2 resolution
Cascade 3+: Progressive LOD reduction
```

**Issues**: Temporal stability, bandwidth problems, sampling artifacts.

### Known Algorithm Problems
- Angular sampling inconsistent across cascades
- Temporal accumulation causes flickering
- Distance range overlaps not handled well
- Performance scaling poor with cascade count
- Memory usage excessive

## Contributing & Experimentation

**This is a vibe coding research project** - contributions welcome but expect:
- Broken builds on some systems (builds are suggestions)
- Frequent API changes (consistency is overrated)
- Incomplete documentation (code is self-documenting, right?)
- Performance issues (optimization is for quitters)
- Shader compilation failures (shaders have feelings too)
- Commits with messages like "it works now" or "fixed the thing"
- Features that exist because they seemed cool at 2 AM

**Contributing Guidelines:**
- Does it compile? Ship it.
- Does it look cool? Definitely ship it.
- Performance regression? That's tomorrow's problem.
- Breaking changes? Every change is a breaking change if you're brave enough.

Good for learning/experimentation, terrible for production use. Perfect for vibe coding.

## Credits & References

**Original Paper**: "Radiance Cascades: A Novel Approach to Calculating Global Illumination [WIP]" by Alexander Sannikov

**Implementation**: Experimental research prototype exploring the paper's concepts

**Third-party Libraries**:
- OpenGL for rendering
- GLFW for windowing
- GLM for mathematics
- ImGui for interface
- stb_image for texture loading

## Disclaimer

This software is provided "as-is" for educational, research, and **vibe coding experimentation** purposes. Expect bugs, crashes, performance issues, incomplete features, and questionable architectural decisions. This is definitely not production software.

The implementation may not accurately reflect the original paper's intent or methodology, but it might look cool while failing. Built with the philosophy that "working software" is a social construct.

Use at your own risk for learning and experimentation only. Side effects may include: appreciation for robust software engineering practices, existential questions about code quality, and an urge to refactor everything. 
