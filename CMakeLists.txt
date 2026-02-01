cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(wen)

set(CMAKE_CXX_STANDARD 20)

set(WEN_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(3rdlibs)
add_subdirectory(engine)

option(WEN_BUILD_SANDBOX "build sandbox" ON)

if(WEN_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()