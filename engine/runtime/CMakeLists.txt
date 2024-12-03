set(TARGET_NAME runtime)

file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.hpp")

add_library(${TARGET_NAME} STATIC ${SRC} ${HEADERS})

target_include_directories(${TARGET_NAME} PUBLIC include)
target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog glfw)
target_precompile_headers(${TARGET_NAME} PUBLIC "include/pch.hpp")