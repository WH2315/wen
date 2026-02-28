#pragma once

#include "function/render/interface/manager.hpp"
#include "function/render/interface/basic/basic.hpp"
#include <vk_mem_alloc.h>

namespace wen::Renderer {

class Context final {
public:
    ~Context();

    static void init();
    static Context& context() { return *instance_; }
    static void quit();

    void initialize();
    void destroy();

    void recreateSwapchain();

public:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    std::unique_ptr<Device> device;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<CommandPool> command_pool;
    std::unique_ptr<DescriptorPool> descriptor_pool;
    VmaAllocator vma_allocator;

    vk::detail::DispatchLoaderDynamic dispatcher;

private:
    void createInstance();
    void createSurface();
    void createVmaAllocator();

private:
    Context();

    static std::unique_ptr<Context> instance_;
};

}  // namespace wen::Renderer