project(tinyobjloader)

set(tinyobjloader-Source
    ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader.cc
)

add_library(${PROJECT_NAME} ${tinyobjloader-Source})