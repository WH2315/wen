set(TARGET_NAME runtime)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    organize_files("src")
    organize_files("include")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

add_library(${TARGET_NAME} STATIC ${sources} ${headers})

target_include_directories(${TARGET_NAME} PUBLIC
    ./include
    ../../3rdlibs/stb
    ../../3rdlibs/VulkanMemoryAllocator
    ../../3rdlibs/tinyobjloader
)
target_link_libraries(${TARGET_NAME} PUBLIC
    spdlog
    glfw
    glm
    Vulkan::Vulkan
    VulkanMemoryAllocator
    tinygltf
    imgui
)
target_link_libraries(${TARGET_NAME} PRIVATE
    glslang glslang::SPIRV glslang::glslang-default-resource-limits
    tinyobjloader
)
target_precompile_headers(${TARGET_NAME} PUBLIC "include/pch.hpp")