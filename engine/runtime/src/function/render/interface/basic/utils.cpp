#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/context.hpp"
#include "core/base/macro.hpp"
#include <fstream>

namespace wen::Renderer {

std::string readFile(const std::string& filename) {
    std::string result;
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        if (size != -1) {
            result.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(&result[0], size);
            file.close();
        } else {
            WEN_CORE_ERROR("Could not read file '{0}'", filename)
        }
    } else {
        WEN_CORE_ERROR("Could not open file '{0}'", filename)
    }
    return result;
}

vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (auto format : candidates) {
        auto props = manager->device->physical_device.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return vk::Format::eUndefined;
}

vk::Format findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, uint32_t level_count, uint32_t layer_count, vk::ImageViewType view_type) {
    vk::ImageViewCreateInfo image_view_ci;

    vk::ImageSubresourceRange subresource_range;
    subresource_range.setAspectMask(aspect)
        .setBaseMipLevel(0)
        .setLevelCount(level_count)
        .setBaseArrayLayer(0)
        .setLayerCount(layer_count);

    vk::ComponentMapping components;
    components.setR(vk::ComponentSwizzle::eR)
        .setG(vk::ComponentSwizzle::eG)
        .setB(vk::ComponentSwizzle::eB)
        .setA(vk::ComponentSwizzle::eA);

    image_view_ci.setImage(image)
        .setFormat(format)
        .setViewType(view_type)
        .setComponents(components)
        .setSubresourceRange(subresource_range);

    return manager->device->device.createImageView(image_view_ci);
}

void transitionImageLayout(vk::Image image, vk::ImageAspectFlagBits aspect, uint32_t mip_levels, const TransitionInfo& src, const TransitionInfo& dst) {
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(src.layout)
        .setSrcAccessMask(src.access)
        .setNewLayout(dst.layout)
        .setDstAccessMask(dst.access)
        .setImage(image)
        .setSubresourceRange({
            aspect,
            0,
            mip_levels,
            0,
            1
        })
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
    
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    cmdbuf.pipelineBarrier(
        src.stage,
        dst.stage,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );
    manager->command_pool->freeSingleUse(cmdbuf);
}

vk::DeviceAddress getBufferAddress(vk::Buffer buffer) {
    vk::BufferDeviceAddressInfo info{};
    info.setBuffer(buffer);
    return manager->device->device.getBufferAddress(info);
}

vk::DeviceAddress getAccelerationStructureAddress(vk::AccelerationStructureKHR as) {
    vk::AccelerationStructureDeviceAddressInfoKHR info{};
    info.setAccelerationStructure(as);
    return manager->device->device.getAccelerationStructureAddressKHR(info, manager->dispatcher);
}

}  // namespace wen::Renderer