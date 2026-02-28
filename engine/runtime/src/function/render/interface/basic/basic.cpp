#include "function/render/interface/basic/basic.hpp"
#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/context.hpp"
#include "core/base/macro.hpp"

namespace wen::Renderer {

Device::Device() {
    auto devices = manager->instance.enumeratePhysicalDevices();

    bool selected = false;
    for (const auto& candidate : devices) {
        if (suitable(candidate)) {
            selected = true;
            break;
        }
    }
    if (!selected) {
        WEN_CORE_ERROR("No suitable physical device found!")
    } else {
        auto properties = physical_device.getProperties();
        WEN_CORE_INFO("Selected GPU: {}", static_cast<std::string>(properties.deviceName.data()))
        if (renderer_config.debug) {
            WEN_CORE_DEBUG("  GPU Driver Version: {}.{}.{}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion))
            WEN_CORE_DEBUG("  GPU API Version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion))
            auto memory_properties = physical_device.getMemoryProperties();
            for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++) {
                auto size = static_cast<float>(memory_properties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
                if (memory_properties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
                    WEN_CORE_DEBUG("  Local GPU Memory: {} Gib", size)
                } else {
                    WEN_CORE_DEBUG("  Shared GPU Memory: {} Gib", size)
                }
            }
        }
    }

    vk::DeviceCreateInfo device_ci;

    vk::PhysicalDeviceFeatures features;
    features.setSamplerAnisotropy(true)
        .setDepthClamp(true)
        .setSampleRateShading(true)
        .setShaderInt64(true)
        .setFragmentStoresAndAtomics(true)
        .setMultiDrawIndirect(true)
        .setGeometryShader(true)
        .setFillModeNonSolid(true)
        .setWideLines(true);
    device_ci.setPEnabledFeatures(&features);
    vk::PhysicalDeviceVulkan12Features features12;
    features12.setRuntimeDescriptorArray(true)
        .setBufferDeviceAddress(true)
        .setShaderSampledImageArrayNonUniformIndexing(true)
        .setDescriptorBindingVariableDescriptorCount(true)
        .setDrawIndirectCount(true)
        .setSamplerFilterMinmax(true);
    device_ci.setPNext(&features12);

    std::map<std::string, bool> requiredExtensions = {
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME, false},
        {VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, false},
    };

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = {};
    vk::PhysicalDeviceRayQueryFeaturesKHR ray_query_features = {};
    if (renderer_config.is_enable_ray_tracing) {
        requiredExtensions.insert(std::make_pair(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, false));
        requiredExtensions.insert(std::make_pair(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false));
        requiredExtensions.insert(std::make_pair(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false));
        requiredExtensions.insert(std::make_pair(VK_KHR_RAY_QUERY_EXTENSION_NAME, false));
        acceleration_structure_features.accelerationStructure = VK_TRUE;
        acceleration_structure_features.accelerationStructureCaptureReplay = VK_TRUE;
        acceleration_structure_features.accelerationStructureIndirectBuild = VK_FALSE;
        acceleration_structure_features.accelerationStructureHostCommands = VK_FALSE;
        acceleration_structure_features.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
        ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;
        ray_tracing_pipeline_features.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
        ray_tracing_pipeline_features.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
        ray_tracing_pipeline_features.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
        ray_tracing_pipeline_features.rayTraversalPrimitiveCulling = VK_TRUE;
        ray_query_features.rayQuery = VK_TRUE;
        device_ci.setPNext(&acceleration_structure_features.setPNext(&ray_tracing_pipeline_features.setPNext(&ray_query_features.setPNext(&features12))));
    }

    std::vector<const char*> extensions;
    auto eps = physical_device.enumerateDeviceExtensionProperties();
    for (const auto& ep : eps) {
        if (requiredExtensions.contains(ep.extensionName)) {
            requiredExtensions[ep.extensionName] = true;
            extensions.push_back(ep.extensionName);
        }
    }
    for (const auto& [ep, success] : requiredExtensions) {
        if (!success) {
            WEN_CORE_ERROR("unsupported device extension: {}", ep)
        }
    }
    device_ci.setPEnabledExtensionNames(extensions);

    const char* layer = "VK_LAYER_KHRONOS_validation";
    if (renderer_config.debug) {
        auto lps = physical_device.enumerateDeviceLayerProperties();
        auto it = std::ranges::find_if(lps, [layer](const auto& lp) {
            return strcmp(lp.layerName, layer) == 0;
        });
        if (it != lps.end()) {
            device_ci.setPEnabledLayerNames(layer);
        } else {
            WEN_CORE_ERROR("Device validation layer not support!")
        }
    }

    std::set<uint32_t> queue_family_indices = {
        graphics_queue_family,
        present_queue_family,
        transfer_queue_family,
        compute_queue_family
    };
    std::vector<vk::DeviceQueueCreateInfo> queue_cis(queue_family_indices.size());
    size_t i = 0;
    float priorities = 1.0f;
    for (auto index : queue_family_indices) {
        queue_cis[i].setQueueFamilyIndex(index)
            .setQueueCount(1)
            .setQueuePriorities(priorities);
        i++;
    }
    device_ci.setQueueCreateInfos(queue_cis);

    device = physical_device.createDevice(device_ci);

    graphics_queue = device.getQueue(graphics_queue_family, 0);
    present_queue = device.getQueue(present_queue_family, 0);
    transfer_queue = device.getQueue(transfer_queue_family, 0);
    compute_queue = device.getQueue(compute_queue_family, 0);

    if (renderer_config.is_enable_ray_tracing) {
        vk::PhysicalDeviceProperties2 properties = {};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {};
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties = {};
        properties.pNext = &acceleration_structure_properties;
        acceleration_structure_properties.pNext = &ray_tracing_pipeline_properties;
        physical_device.getProperties2(&properties);
        WEN_CORE_DEBUG("  the maximum number of geometries in the bottom level acceleration structure: {}", acceleration_structure_properties.maxGeometryCount)
        WEN_CORE_DEBUG("  the maximum number of instances in the top level acceleration structure: {}", acceleration_structure_properties.maxInstanceCount)
        WEN_CORE_DEBUG("  the maximum number of levels of ray recursion allowed in a trace command: {}", ray_tracing_pipeline_properties.maxRayRecursionDepth)
    }
}

Device::~Device() {
    physical_device = nullptr;
    device.destroy();
}

bool Device::suitable(const vk::PhysicalDevice& candidate) {
    auto properties = candidate.getProperties();

    std::string device_name = properties.deviceName;
    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
        WEN_CORE_ERROR("PhysicalDevice: {} not a discrete GPU", device_name)
        return false;
    }

    bool found = false;
    auto qfproperties = candidate.getQueueFamilyProperties();
    auto flags = vk::QueueFlagBits::eGraphics|vk::QueueFlagBits::eTransfer|vk::QueueFlagBits::eCompute;
    for (uint32_t i = 0; i < qfproperties.size(); i++) {
        auto present_support = candidate.getSurfaceSupportKHR(i, manager->surface);
        if (qfproperties[i].queueFlags & flags && present_support) {
            graphics_queue_family = i;
            present_queue_family = i;
            transfer_queue_family = i;
            compute_queue_family = i;
            found = true;
            break;
        }
    }
    if (!found) {
        WEN_CORE_ERROR("PhysicalDevice: {} not support required queue family", device_name)
        return false;
    }

    physical_device = candidate;
    return true;
}

Swapchain::Swapchain() {
    vk::SwapchainKHR old_swapchain = swapchain;

    SwapchainSupportDetails details = {
        .capabilities = manager->device->physical_device.getSurfaceCapabilitiesKHR(manager->surface),
        .formats = manager->device->physical_device.getSurfaceFormatsKHR(manager->surface),
        .modes = manager->device->physical_device.getSurfacePresentModesKHR(manager->surface)
    };

    format = details.formats[0];
    mode = details.modes[0];

    vk::SurfaceFormatKHR desired_format;
    desired_format = {vk::Format::eB8G8R8A8Unorm,
                      vk::ColorSpaceKHR::eSrgbNonlinear};
    for (auto format : details.formats) {
        if (format.format == desired_format.format &&
            format.colorSpace == desired_format.colorSpace) {
            this->format = format;
            break;
        }
    }

    vk::PresentModeKHR desired_mode;
    if (renderer_config.vsync) {
        desired_mode = vk::PresentModeKHR::eFifo;
    } else {
        desired_mode = vk::PresentModeKHR::eMailbox;
    }
    for (auto mode : details.modes) {
        if (mode == desired_mode) {
            this->mode = mode;
            break;
        }
    }

    uint32_t desired_image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 &&
        desired_image_count > details.capabilities.maxImageCount) {
        desired_image_count = details.capabilities.maxImageCount;
    }

    uint32_t width =
        std::clamp<uint32_t>(details.capabilities.currentExtent.width,
                             details.capabilities.minImageExtent.width,
                             details.capabilities.maxImageExtent.width);
    uint32_t height =
        std::clamp<uint32_t>(details.capabilities.currentExtent.height,
                             details.capabilities.minImageExtent.height,
                             details.capabilities.maxImageExtent.height);
    renderer_config.swapchain_image_width = width;
    renderer_config.swapchain_image_height = height;

    vk::SwapchainCreateInfoKHR swapchain_ci;
    swapchain_ci.setOldSwapchain(old_swapchain)
        .setSurface(manager->surface)
        .setPreTransform(details.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(mode)
        .setClipped(true)
        .setMinImageCount(desired_image_count)
        .setImageFormat(format.format)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent({width, height})
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    uint32_t queue_family_indices[] = {manager->device->graphics_queue_family,
                                       manager->device->present_queue_family};
    if (queue_family_indices[0] != queue_family_indices[1]) {
        swapchain_ci.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(queue_family_indices);
    } else {
        swapchain_ci.setImageSharingMode(vk::SharingMode::eExclusive);
    }

    swapchain = manager->device->device.createSwapchainKHR(swapchain_ci);

    if (old_swapchain != nullptr) {
        for (auto& image_view : image_views) {
            manager->device->device.destroyImageView(image_view);
        }
        image_views.clear();
        manager->device->device.destroySwapchainKHR(old_swapchain);
    }

    images = manager->device->device.getSwapchainImagesKHR(swapchain);
    image_count = static_cast<uint32_t>(images.size());
    image_views.resize(image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        image_views[i] = createImageView(images[i], format.format,
                                         vk::ImageAspectFlagBits::eColor, 1);
    }
}

Swapchain::~Swapchain() {
    for (auto& image_view : image_views) {
        manager->device->device.destroyImageView(image_view);
    }
    image_views.clear();
    manager->device->device.destroySwapchainKHR(swapchain);
}

CommandPool::CommandPool(vk::CommandPoolCreateFlags flags) {
    vk::CommandPoolCreateInfo create_info;
    create_info.setFlags(flags)
        .setQueueFamilyIndex(manager->device->graphics_queue_family);
    command_pool_ = manager->device->device.createCommandPool(create_info);
}

CommandPool::~CommandPool() {
    manager->device->device.destroyCommandPool(command_pool_);
}

std::vector<vk::CommandBuffer> CommandPool::allocateCommandBuffers(uint32_t count) {
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool_)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(count);
    return std::move(manager->device->device.allocateCommandBuffers(allocate_info));
}

vk::CommandBuffer CommandPool::allocateSingleUse() {
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool_)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    auto cmdbuf = manager->device->device.allocateCommandBuffers(allocate_info)[0];
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdbuf.begin(begin_info);

    return cmdbuf;
}

void CommandPool::freeSingleUse(vk::CommandBuffer cmdbuf) {
    cmdbuf.end();

    vk::SubmitInfo submits = {};
    submits.setCommandBuffers(cmdbuf);
    manager->device->transfer_queue.submit(submits, nullptr);
    manager->device->transfer_queue.waitIdle();
    manager->device->device.freeCommandBuffers(command_pool_, cmdbuf);
}

DescriptorPool::DescriptorPool() {
    std::vector<DescriptorPool::PoolSizeRatio> pool_ratios = {
        {vk::DescriptorType::eUniformBuffer, 1.0f},
        {vk::DescriptorType::eCombinedImageSampler, 1.0f},
        {vk::DescriptorType::eStorageBuffer, 1.0f},
        {vk::DescriptorType::eStorageImage, 1.0f}
    };
    if (renderer_config.is_enable_ray_tracing) {
        pool_ratios.push_back({vk::DescriptorType::eAccelerationStructureKHR, 1.0f});
    }
    init(10, pool_ratios);
}

DescriptorPool::~DescriptorPool() {
    clearPools();
    for (auto& pool : ready_pools_) {
        manager->device->device.destroyDescriptorPool(pool);
	}
    ready_pools_.clear();
	for (auto& pool : full_pools_) {
        manager->device->device.destroyDescriptorPool(pool);
    }
    full_pools_.clear();
}

void DescriptorPool::init(uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios) {
    ratios_.clear();    
    for (auto ratio : pool_ratios) {
        ratios_.push_back(ratio);
    }
    auto new_pool = createPool(max_sets, pool_ratios);
    sets_per_pool_ = max_sets * 1.5;
    ready_pools_.push_back(new_pool);
}

void DescriptorPool::clearPools() {
    for (auto pool : ready_pools_) {
        manager->device->device.resetDescriptorPool(pool);
    }
    for (auto pool : full_pools_) {
        manager->device->device.resetDescriptorPool(pool);
        ready_pools_.push_back(pool);
    }
    full_pools_.clear();
}

std::vector<vk::DescriptorSet> DescriptorPool::allocate(vk::DescriptorSetLayout layout) {
    auto pool_to_use = getPool();
    std::vector<vk::DescriptorSet> descriptor_sets(renderer_config.max_frames_in_flight);
    std::vector<vk::DescriptorSetLayout> layouts(renderer_config.max_frames_in_flight, layout);
    vk::DescriptorSetAllocateInfo allocate_info;
    allocate_info.setDescriptorPool(pool_to_use)
        .setDescriptorSetCount(static_cast<uint32_t>(renderer_config.max_frames_in_flight))
        .setSetLayouts(layouts);
    try {
        descriptor_sets = manager->device->device.allocateDescriptorSets(allocate_info);
    } catch (vk::OutOfPoolMemoryError) {
        full_pools_.push_back(pool_to_use);
        pool_to_use = getPool();
        allocate_info.setDescriptorPool(pool_to_use);
        descriptor_sets = manager->device->device.allocateDescriptorSets(allocate_info);
    }
    ready_pools_.push_back(pool_to_use);
    return descriptor_sets;
}

void DescriptorPool::free(const std::vector<vk::DescriptorSet>& descriptor_sets) {
    auto pool_to_use = getPool();
    manager->device->device.freeDescriptorSets(pool_to_use, descriptor_sets);
    ready_pools_.push_back(pool_to_use);
}

vk::DescriptorPool DescriptorPool::createPool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) {
    std::vector<vk::DescriptorPoolSize> pool_size;
    for (auto ratio : pool_ratios) {
        pool_size.push_back({
            ratio.descriptor_type,
            static_cast<uint32_t>(set_count * ratio.ratio)
        });
    }
    vk::DescriptorPoolCreateInfo create_info;
    create_info.setPoolSizeCount(static_cast<uint32_t>(pool_size.size()))
        .setPoolSizes(pool_size)
        .setMaxSets(set_count)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    return manager->device->device.createDescriptorPool(create_info);
}

vk::DescriptorPool DescriptorPool::getPool() {
    vk::DescriptorPool new_pool;
    if (!ready_pools_.empty()) {
        new_pool = ready_pools_.back();
        ready_pools_.pop_back();
    } else {
        new_pool = createPool(sets_per_pool_, ratios_);
        sets_per_pool_ *= 1.5;
        if (sets_per_pool_ > 4092) {
            sets_per_pool_ = 4092;
        }
    }
    return new_pool;
}

}  // namespace wen::Renderer