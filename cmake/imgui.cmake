project(imgui)
file(GLOB SRC
    3rdlibs/imgui/*.cpp
    3rdlibs/imgui/backends/imgui_impl_glfw.cpp
    3rdlibs/imgui/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${SRC})
target_include_directories(imgui PUBLIC 3rdlibs/imgui)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)