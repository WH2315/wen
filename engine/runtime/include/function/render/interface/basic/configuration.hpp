#pragma once

#include <vulkan/vulkan.hpp>

namespace wen::Renderer {

struct Configuration {
    std::string app_name = "wen app";
    std::string engine_name = "wen engine";
    bool debug = false;
    bool vsync = false;
    uint32_t swapchain_image_width;
    uint32_t swapchain_image_height;
    bool is_enable_ray_tracing = true;
    uint32_t max_frames_in_flight = 2;
    uint32_t current_frame_in_flight = 0;
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;
    bool msaa() const { return msaa_samples != vk::SampleCountFlagBits::e1; }
    // LOD 调试:按当前选中的 LOD 层级给网格着色(默认关闭,编辑器 Setting 面板可勾选)。
    bool lod_debug_enabled = false;
};

}  // namespace wen::Renderer