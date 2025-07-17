
cmake_minimum_required(VERSION 3.10)
project(vibe-gi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(vibe-gi ${SOURCES})

target_link_libraries(vibe-gi OpenGL::GL glfw glm::glm Freetype::Freetype)
add_custom_command(TARGET vibe-gi POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:vibe-gi>/shaders) 