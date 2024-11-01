set(ENGINE_ROOT_DIR "${WEN_ROOT_DIR}/engine")

include(${WEN_ROOT_DIR}/3rdlibs/vulkan.cmake)

add_subdirectory(runtime)
add_subdirectory(editor)