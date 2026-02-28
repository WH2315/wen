#pragma once

#include <vulkan/vulkan.hpp>

namespace wen::Renderer {

std::string readFile(const std::string& filename);

vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
vk::Format findDepthFormat();

vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, uint32_t level_count, uint32_t layer_count = 1, vk::ImageViewType view_type = vk::ImageViewType::e2D);

template <typename DstType, typename SrcType>
DstType convert(SrcType src);

struct TransitionInfo {
    vk::ImageLayout layout;
    vk::AccessFlags access;
    vk::PipelineStageFlags stage;
};
void transitionImageLayout(vk::Image image, vk::ImageAspectFlagBits aspect, uint32_t mip_levels, const TransitionInfo& src, const TransitionInfo& dst);

vk::DeviceAddress getBufferAddress(vk::Buffer buffer);
vk::DeviceAddress getAccelerationStructureAddress(vk::AccelerationStructureKHR as);

}  // namespace wen::Renderer