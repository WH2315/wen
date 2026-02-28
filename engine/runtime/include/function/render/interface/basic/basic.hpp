#pragma once

#include <vulkan/vulkan.hpp>

namespace wen::Renderer {

class Device {
public:
    Device();
    ~Device();

public:
    vk::PhysicalDevice physical_device;
    vk::Device device;

    vk::Queue graphics_queue;
    uint32_t graphics_queue_family = -1;
    vk::Queue present_queue;
    uint32_t present_queue_family = -1;
    vk::Queue transfer_queue;
    uint32_t transfer_queue_family = -1;
    vk::Queue compute_queue;
    uint32_t compute_queue_family = -1;

private:
    bool suitable(const vk::PhysicalDevice& candidate);
};

class Swapchain {
public:
    Swapchain();
    ~Swapchain();

public:
    vk::SwapchainKHR swapchain = nullptr;
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR mode;

    uint32_t image_count;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;

private:
    struct SwapchainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> modes;
    };
};

class CommandPool {
    friend class KtxTexture;

public:
    CommandPool(vk::CommandPoolCreateFlags flags);
    ~CommandPool();

    std::vector<vk::CommandBuffer> allocateCommandBuffers(uint32_t count);

    vk::CommandBuffer allocateSingleUse();
    void freeSingleUse(vk::CommandBuffer cmdbuf);

private:
    vk::CommandPool command_pool_;
};

class DescriptorPool {
public:
    struct PoolSizeRatio {
        vk::DescriptorType descriptor_type;
        float ratio;
    };

    DescriptorPool();
    ~DescriptorPool();

    void init(uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void clearPools();

    std::vector<vk::DescriptorSet> allocate(vk::DescriptorSetLayout layout);
    void free(const std::vector<vk::DescriptorSet>& descriptor_sets);

private:
    vk::DescriptorPool createPool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);
    vk::DescriptorPool getPool();

private:
    std::vector<PoolSizeRatio> ratios_;
    std::vector<vk::DescriptorPool> full_pools_;
    std::vector<vk::DescriptorPool> ready_pools_;
    uint32_t sets_per_pool_;
};

}  // namespace wen::Renderer