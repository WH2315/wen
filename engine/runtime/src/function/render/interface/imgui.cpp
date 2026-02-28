#include "function/render/interface/imgui.hpp"
#include "engine/global_context.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace wen::Renderer {

Imgui::Imgui(Renderer& renderer) : renderer_(renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize.x = static_cast<float>(renderer_config.swapchain_image_width);
    io.DisplaySize.y = static_cast<float>(renderer_config.swapchain_image_height);

    ImGui::StyleColorsClassic();
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

    std::string src = renderer.render_pass->subpasses.back()->name;
    auto& subpass = renderer.render_pass->addSubpass("imgui_subpass");
    subpass.setOutputAttachment(SWAPCHAIN_IMAGE_ATTACHMENT); 
    renderer.render_pass->addSubpassDependency(
        src,
        "imgui_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite
        }    
    );
    renderer.updateRenderPass();

    ImFontConfig config;
    io.Fonts->AddFontFromFileTTF("sandbox/resources/JetBrainsMonoNLNerdFontMono-Bold.ttf", 18.0f, &config, io.Fonts->GetGlyphRangesDefault());

    ImGui_ImplGlfw_InitForVulkan(global_context->window_system->getRuntimeWindow()->getWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = manager->instance;
    init_info.PhysicalDevice = manager->device->physical_device;
    init_info.Device = manager->device->device;
    init_info.QueueFamily = manager->device->graphics_queue_family;
    init_info.Queue = manager->device->graphics_queue;
    init_info.PipelineInfoMain.RenderPass = renderer.render_pass->render_pass;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = descriptor_pool_;
    init_info.PipelineInfoMain.Subpass = renderer.render_pass->subpasses.size() - 1;
    init_info.MinImageCount = manager->swapchain->image_count;
    init_info.ImageCount = manager->swapchain->image_count;
    init_info.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(renderer_config.msaa_samples);
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](VkResult result) {
        if (result != VK_SUCCESS) {
            WEN_CORE_ERROR("ImGui Vulkan Error: {}", static_cast<uint32_t>(result))
        }
    };
    ImGui_ImplVulkan_Init(&init_info);
}

void Imgui::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Imgui::renderFrame() {
    renderer_.nextSubpass("imgui_subpass");
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderer_.getCurrentBuffer());
}

Imgui::~Imgui() {
    manager->device->device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    manager->device->device.destroyDescriptorPool(descriptor_pool_);
    ImGui::DestroyContext();
}

}  // namespace wen::Renderer