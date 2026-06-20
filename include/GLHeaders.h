// Central OpenGL header.
//
// The project targets OpenGL 4.3+ core (Linux/Windows). OpenGL function
// pointers are loaded at runtime via the vendored GLAD loader, so this header
// must be included before GLFW (or anything that pulls in gl.h). Building with
// GLFW_INCLUDE_NONE (set in CMake) makes the include order irrelevant.
//
// Note: macOS is intentionally unsupported - Apple's OpenGL is frozen at 4.1
// (no compute shaders / SSBOs). A Metal backend is the planned path there.
#pragma once

#include <glad/glad.h>
