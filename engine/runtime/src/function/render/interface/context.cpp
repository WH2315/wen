#include "function/render/interface/context.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"

namespace wen::Renderer {

std::unique_ptr<Context> Context::instance_ = nullptr;

Context::Context() {}

Context::~Context() {}

void Context::init() {
    if (!instance_) {
        instance_.reset(new Context);
    }
}

void Context::quit() { instance_.reset(); }

void Context::initialize() {
    createInstance();
    if (renderer_config.is_enable_ray_tracing) {
        vk::detail::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        dispatcher = vk::detail::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
    }
    createSurface();
    device = std::make_unique<Device>();
    swapchain = std::make_unique<Swapchain>();
    command_pool = std::make_unique<CommandPool>(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    descriptor_pool = std::make_unique<DescriptorPool>();
    createVmaAllocator();
}

void Context::destroy() {
    vmaDestroyAllocator(vma_allocator);
    descriptor_pool.reset();
    command_pool.reset();
    swapchain.reset();
    device.reset();
    instance.destroySurfaceKHR(surface);
    instance.destroy();
}

void Context::createInstance() {
    vk::ApplicationInfo app_info;
    app_info.setPApplicationName(renderer_config.app_name.c_str())
        .setApplicationVersion(1)
        .setPEngineName(renderer_config.engine_name.c_str())
        .setEngineVersion(1)
        .setApiVersion(vk::ApiVersion14);

    vk::InstanceCreateInfo ins_ci;
    ins_ci.setPApplicationInfo(&app_info);

    std::map<std::string, bool> requiredExtensions;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.insert(std::make_pair(static_cast<std::string>(glfwExtensions[i]), false));
    }

    std::vector<const char*> extensions;
    auto eps = vk::enumerateInstanceExtensionProperties();
    for (const auto& ep : eps) {
        if (requiredExtensions.contains(ep.extensionName)) {
            requiredExtensions[ep.extensionName] = true;
            extensions.push_back(ep.extensionName);
        }
    }
    for (const auto& [ep, success] : requiredExtensions) {
        if (!success) {
            WEN_CORE_ERROR("unsupported instance extension: {}", ep)
        }
    }
    ins_ci.setPEnabledExtensionNames(extensions);

    const char* layer = "VK_LAYER_KHRONOS_validation";
    if (renderer_config.debug) {
        auto lps = vk::enumerateInstanceLayerProperties();
        auto it = std::ranges::find_if(lps, [layer](const auto& lp) {
            return strcmp(lp.layerName, layer) == 0;
        });
        if (it != lps.end()) {
            ins_ci.setPEnabledLayerNames(layer);
        } else {
            WEN_CORE_ERROR("Instance validation layer not support!")
        }
    }

    instance = vk::createInstance(ins_ci);
}

void Context::createSurface() {
    VkSurfaceKHR vk_surface;
    glfwCreateWindowSurface(instance, global_context->window_system->getRuntimeWindow()->getWindow(), nullptr, &vk_surface);
    surface = static_cast<vk::SurfaceKHR>(vk_surface);
}

void Context::createVmaAllocator() {
    VmaAllocatorCreateInfo vma_alloc_ci{};
    if (renderer_config.is_enable_ray_tracing) {
        vma_alloc_ci.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }
    vma_alloc_ci.vulkanApiVersion = VK_API_VERSION_1_4;
    vma_alloc_ci.instance = instance;
    vma_alloc_ci.physicalDevice = device->physical_device;
    vma_alloc_ci.device = device->device;
    vmaCreateAllocator(&vma_alloc_ci, &vma_allocator);
}

void Context::recreateSwapchain() {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(global_context->window_system->getRuntimeWindow()->getWindow(), &width, &height);
        glfwWaitEvents();
    }
    device->device.waitIdle();
    swapchain.reset();
    swapchain = std::make_unique<Swapchain>();
}

}  // namespace wen::Renderer