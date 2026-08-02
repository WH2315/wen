project(imguizmo)
add_library(imguizmo STATIC 3rdlibs/ImGuizmo/ImGuizmo.cpp)
target_include_directories(imguizmo PUBLIC 3rdlibs/ImGuizmo)
target_link_libraries(imguizmo PUBLIC imgui)
