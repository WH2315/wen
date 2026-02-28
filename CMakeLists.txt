cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(wen)

set(CMAKE_CXX_STANDARD 20)

set(WEN_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ParserPath ${WEN_ROOT_DIR}/engine/parser)

include(cmake/vulkan.cmake)
include(cmake/3rdlibs.cmake)
include(cmake/imgui.cmake)

add_subdirectory(engine)

option(WEN_BUILD_SANDBOX "build sandbox" ON)
option(WEN_BUILD_EXAMPLE "build example" OFF)

if(WEN_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(WEN_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()