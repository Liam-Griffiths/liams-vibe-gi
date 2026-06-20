
cmake_minimum_required(VERSION 3.10)
project(vibe-gi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${CMAKE_SOURCE_DIR}/third_party/imgui/backends)
include_directories(${CMAKE_SOURCE_DIR}/scripts)

# GLAD OpenGL loader (OpenGL 4.6 core). Provides the GL function pointers at
# runtime on Linux/Windows.
include_directories(${CMAKE_SOURCE_DIR}/third_party/glad/include)
set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB SCRIPT_SOURCES "${CMAKE_SOURCE_DIR}/scripts/*.cpp")

# ImGui source files
set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(vibe-gi ${SOURCES} ${SCRIPT_SOURCES} ${IMGUI_SOURCES} ${GLAD_SOURCES})

# Prevent GLFW from including the system OpenGL headers so that the GLAD loader
# (included via include/GLHeaders.h) is the single source of GL symbols and
# include ordering does not matter.
target_compile_definitions(vibe-gi PRIVATE GLFW_INCLUDE_NONE)

target_link_libraries(vibe-gi OpenGL::GL glfw glm::glm ${CMAKE_DL_LIBS})
add_custom_command(TARGET vibe-gi POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:vibe-gi>/shaders)

# Create symlinks for asset directories
add_custom_command(TARGET vibe-gi POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink
                   ${CMAKE_SOURCE_DIR}/models $<TARGET_FILE_DIR:vibe-gi>/models)

add_custom_command(TARGET vibe-gi POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink
                   ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:vibe-gi>/textures) 