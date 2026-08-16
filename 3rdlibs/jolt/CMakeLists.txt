file(GLOB_RECURSE jolt_sources CONFIGURE_DEPENDS "Jolt/*.cpp")

# DebugRenderer 被 JPH_DEBUG_RENDERER 守卫(本项目未启用, 空 TU 浪费编译时间);
# ComputeSystem 无条件引用 GPU 计算后端, 排除。
list(REMOVE_ITEM jolt_sources
     "${CMAKE_CURRENT_SOURCE_DIR}/Jolt/Renderer/DebugRenderer.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/Jolt/Compute/ComputeSystem.cpp")

add_library(jolt STATIC ${jolt_sources})
target_include_directories(jolt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(jolt PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
