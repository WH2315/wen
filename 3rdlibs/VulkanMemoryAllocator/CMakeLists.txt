project(VulkanMemoryAllocator)

set(VulkanMemoryAllocator-Source
    ${CMAKE_CURRENT_SOURCE_DIR}/vk_mem_alloc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vk_mem_alloc.cpp
)

add_library(${PROJECT_NAME} ${VulkanMemoryAllocator-Source})

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)