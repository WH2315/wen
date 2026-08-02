cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(wen)

set(CMAKE_CXX_STANDARD 20)

# Vulkan uses 0..1 clip-space depth; without this GLM emits OpenGL-style
# -1..1 projection matrices (halving depth precision, near plane ~2x off).
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(WEN_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ParserPath ${WEN_ROOT_DIR}/engine/parser)

include(cmake/vulkan.cmake)
include(cmake/3rdlibs.cmake)
include(cmake/imgui.cmake)
include(cmake/imguizmo.cmake)

add_subdirectory(engine)

option(WEN_BUILD_SANDBOX "build sandbox" ON)
option(WEN_BUILD_EXAMPLE "build example" OFF)

if(WEN_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(WEN_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()