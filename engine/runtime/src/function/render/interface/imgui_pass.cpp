#include "function/render/interface/imgui_pass.hpp"
#include "function/render/interface/context.hpp"
#include "function/render/interface/manager.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace wen::Renderer {

ImguiPass::ImguiPass(Renderer& renderer) : renderer_(renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize.x = static_cast<float>(renderer_config.swapchain_image_width);
    io.DisplaySize.y = static_cast<float>(renderer_config.swapchain_image_height);

    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowMinSize = {160, 160};
    style.WindowRounding = 2;

    std::vector<vk::DescriptorPoolSize> pool_sizes = {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000}
    };
    vk::DescriptorPoolCreateInfo create_info;
    create_info.setPoolSizes(pool_sizes)
        .setMaxSets(pool_sizes.size() * 1000)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descriptor_pool_ = manager->device->device.createDescriptorPool(create_info);

    createRenderPass();
    createFramebuffers();

    io.Fonts->AddFontFromFileTTF("engine/assets/fonts/JetBrainsMonoNLNerdFontMono-Bold.ttf", 26.0f);

    ImGui_ImplGlfw_InitForVulkan(global_context->window_system->getRuntimeWindow()->getWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = manager->instance;
    init_info.PhysicalDevice = manager->device->physical_device;
    init_info.Device = manager->device->device;
    init_info.QueueFamily = manager->device->graphics_queue_family;
    init_info.Queue = manager->device->graphics_queue;
    init_info.PipelineInfoMain.RenderPass = render_pass_;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = descriptor_pool_;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.MinImageCount = manager->swapchain->image_count;
    init_info.ImageCount = manager->swapchain->image_count;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS) {
            WEN_CORE_ERROR("ImGui Vulkan Error: {}", static_cast<uint32_t>(result))
        }
    };
    ImGui_ImplVulkan_Init(&init_info);

    recreate_callback_id_ = renderer_.registerResourceRecreateCallback([this]() {
        destroyFramebuffers();
        createFramebuffers();
    });
}

ImguiPass::~ImguiPass() {
    manager->device->device.waitIdle();
    renderer_.unregisterResourceRecreateCallback(recreate_callback_id_);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    destroyFramebuffers();
    manager->device->device.destroyRenderPass(render_pass_);
    manager->device->device.destroyDescriptorPool(descriptor_pool_);
    ImGui::DestroyContext();
}

void ImguiPass::createRenderPass() {
    vk::AttachmentDescription color;
    color.setFormat(manager->swapchain->format.format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_ref;
    color_ref.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(color_ref);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo create_info;
    create_info.setAttachments(color)
        .setSubpasses(subpass)
        .setDependencies(dependency);
    render_pass_ = manager->device->device.createRenderPass(create_info);
}

void ImguiPass::createFramebuffers() {
    auto width = renderer_config.swapchain_image_width;
    auto height = renderer_config.swapchain_image_height;
    for (auto image_view : manager->swapchain->image_views) {
        vk::FramebufferCreateInfo create_info;
        create_info.setRenderPass(render_pass_)
            .setAttachments(image_view)
            .setWidth(width)
            .setHeight(height)
            .setLayers(1);
        framebuffers_.push_back(manager->device->device.createFramebuffer(create_info));
    }
}

void ImguiPass::destroyFramebuffers() {
    for (auto framebuffer : framebuffers_) {
        manager->device->device.destroyFramebuffer(framebuffer);
    }
    framebuffers_.clear();
}

void ImguiPass::render(Renderer& renderer, const std::function<void()>& ui_callback) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ui_callback) {
        ui_callback();
    }

    ImGui::EndFrame();
    ImGui::Render();

    auto cmdbuf = renderer.getCurrentBuffer();
    vk::ClearValue clear_value;
    clear_value.setColor(vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}));

    vk::RenderPassBeginInfo begin_info;
    begin_info.setRenderPass(render_pass_)
        .setFramebuffer(framebuffers_[renderer.getCurrentImageIndex()])
        .setRenderArea({{0, 0}, {renderer_config.swapchain_image_width, renderer_config.swapchain_image_height}})
        .setClearValues(clear_value);
    cmdbuf.beginRenderPass(begin_info, vk::SubpassContents::eInline);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
    cmdbuf.endRenderPass();
}

}  // namespace wen::Renderer
