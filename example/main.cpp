#include <wen.hpp>
#include <pch.hpp>
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "camera.hpp"
#include "function/render/interface/imgui.hpp"

using namespace wen;

int main() {
    auto engine = std::make_unique<Engine>();

    engine->startupEngine();

    auto interface = std::make_unique<Renderer::Interface>("example/resources");

    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(Renderer::SWAPCHAIN_IMAGE_ATTACHMENT, Renderer::AttachmentType::eColor);
    render_pass->addAttachment(Renderer::DEPTH_ATTACHMENT, Renderer::AttachmentType::eDepth);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(Renderer::SWAPCHAIN_IMAGE_ATTACHMENT);
    subpass.setDepthAttachment(Renderer::DEPTH_ATTACHMENT);

    render_pass->addSubpassDependency(
        Renderer::EXTERNAL_SUBPASS, "main_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );

    render_pass->build();

    auto renderer = interface->createRenderer(std::move(render_pass));

    auto imgui = std::make_unique<Renderer::Imgui>(*renderer);

    auto scene = interface->loadGLTFScene("Sponza/glTF/Sponza.gltf", {"NORMAL", "TEXCOORD_0"});

    struct Material {
        glm::vec3 albedo = glm::vec3(1.0f);
        float roughness = 0.3f;
        glm::vec3 specular_albedo = glm::vec3(1.0f);
        float specular_probability = 0.2f;
        glm::vec3 emissive_color = glm::vec3(1.0f, 0.9f, 0.8f);
        float emissive_intensity = 0.5f;
    };
    auto sphere = interface->createSphereModel();
    sphere->registerCustomSphereData<Material>();
    std::random_device device;
    std::mt19937 generator(device());
    std::normal_distribution<float> distribution(0, 1.0f);
    std::normal_distribution<float> scaleDistribution(0.2f, 0.1f);
    std::uniform_real_distribution<float> colorDistribution(0, 1);
    for (int i = 0; i < 200; i++) {
        float r = scaleDistribution(generator);
        sphere->addSphereModel(
            0,
            {
                distribution(generator) * 4,
                r + (distribution(generator) > 0 ? 4 : 0),
                distribution(generator) * 4
            },
            r,
            Material{
                .albedo = {colorDistribution(generator), colorDistribution(generator), colorDistribution(generator)},
                .roughness = colorDistribution(generator) * colorDistribution(generator),
                .emissive_color = glm::vec3{1},
                .emissive_intensity = colorDistribution(generator) * 1.2f,
            });
    }
    sphere->addSphereModel(0, {-2, 1, 0}, 0.4, Material{.albedo = {1, 0.4, 0.4}, .roughness = 1});
    // 非金属反射
    sphere->addSphereModel(0, {0, 1, 0}, 0.7, Material{.albedo = {0.4, 0.4, 1}, .roughness = 1, .specular_probability = 0.2});
    // 金属反射
    sphere->addSphereModel(0, {2, 1, 0}, 1, Material{.albedo = {0.4, 0.4, 1}, .roughness = 0, .specular_probability = 0});
    // 大地
    sphere->addSphereModel(0, {0, -500, 0}, 500, Material{.albedo = {0.8, 0.5, 0.25}, .roughness = 1.0, .emissive_intensity = 0});

    auto dragon = interface->loadNormalModel("dragon.obj");

    auto as = interface->createAccelerationStructure();
    as->addModel(sphere);
    as->addModel(dragon);
    as->addGLTFScene(scene);
    as->build(false, false);

    auto rt_instance = interface->createRayTracingInstance();
    rt_instance->registerCustomInstanceData<Material>();
    rt_instance->addNormalModel(
        0,
        0,
        sphere,
        glm::mat4(1.0f)
    );
    auto mat = Material{.albedo = glm::vec3(0.8f, 0.5f, 0.25f), .roughness = 0.6f};
    rt_instance->addNormalModel(
        1,
        1,
        dragon,
        glm::translate(glm::vec3(-1, 0.5, 0)),
        mat
    );
    rt_instance->addGLTFScene(2, 2, scene);
    rt_instance->build(true);

    auto width = Renderer::renderer_config.swapchain_image_width;
    auto height = Renderer::renderer_config.swapchain_image_height;

    // camera
    auto camera = std::make_unique<Camera>();
    camera->setViewportSize(width, height);
    camera->setInitialState({2.0f, 2.0f, 0.0f}, {-1.0f, 0.0f, 0.0f});
 
    // push constants
    auto pcs = interface->createPushConstants(
        Renderer::ShaderStage::eRaygen,
        {
            {"time", Renderer::ConstantType::eFloat},
            {"frame_index", Renderer::ConstantType::eInt32},
            {"sample_count", Renderer::ConstantType::eInt32},
            {"view_depth", Renderer::ConstantType::eFloat},
            {"view_depth_strength", Renderer::ConstantType::eFloat},
            {"max_ray_recursion_depth", Renderer::ConstantType::eInt32}
        }
    );

    auto image = interface->createStorageImage(
        width,
        height,
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eSampled
    );
    auto sampler = interface->createSampler();

    auto raygen = interface->loadShader("rt_rgen.glsl", Renderer::ShaderStage::eRaygen);
    auto miss = interface->loadShader("rt_miss.glsl", Renderer::ShaderStage::eMiss);
    auto sphere_rchit = interface->loadShader("sphere_chit.glsl", Renderer::ShaderStage::eClosestHit);
    auto sphere_rint = interface->loadShader("sphere_int.glsl", Renderer::ShaderStage::eIntersection);
    auto triangle_rchit = interface->loadShader("triangle_chit.glsl", Renderer::ShaderStage::eClosestHit);
    auto closest = interface->loadShader("rt_chit.glsl", Renderer::ShaderStage::eClosestHit);

    auto rt_sp = interface->createRayTracingShaderProgram();
    rt_sp->setRaygenShader(raygen);
    rt_sp->setMissShader(miss);
    rt_sp->setHitGroup({sphere_rchit, sphere_rint});
    rt_sp->setHitGroup({triangle_rchit, std::nullopt});
    rt_sp->setHitGroup({closest, std::nullopt});
    auto rt_ds = interface->createDescriptorSet();
    rt_ds->addDescriptors(
        {
            // camera
            {0, vk::DescriptorType::eUniformBuffer, Renderer::ShaderStage::eRaygen},
            // rt_instance
            {1, vk::DescriptorType::eAccelerationStructureKHR, Renderer::ShaderStage::eRaygen | Renderer::ShaderStage::eClosestHit},
            // output image
            {2, vk::DescriptorType::eStorageImage, Renderer::ShaderStage::eRaygen},
            // sphere data buffer
            {3, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit | Renderer::ShaderStage::eIntersection},
            // custom sphere material data buffer
            {4, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // instance address buffer
            {5, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // custom material data buffer
            {6, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // GLTF: primitive data buffer
            {7, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // GLTF: material buffer
            {8, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // GLTF: normal buffer
            {9, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // GLTF: texcoord_0 buffer
            {10, vk::DescriptorType::eStorageBuffer, Renderer::ShaderStage::eClosestHit},
            // GLTF: all textures
            {11, vk::DescriptorType::eCombinedImageSampler, scene->getTexturesCount(), Renderer::ShaderStage::eClosestHit}
        }
    );
    rt_ds->build();
    rt_ds->bindUniform(0, camera->uniform_buffer);
    rt_ds->bindAccelerationStructure(1, rt_instance);
    rt_ds->bindStorageImage(2, image);
    rt_ds->bindStorageBuffer(3, sphere->getSphereDataBuffer());
    rt_ds->bindStorageBuffer(4, sphere->getCustomSphereDataBuffer<Material>());
    rt_ds->bindStorageBuffer(5, rt_instance->getInstanceAddressBuffer());
    rt_ds->bindStorageBuffer(6, rt_instance->getCustomInstanceDataBuffer<Material>());
    rt_ds->bindStorageBuffer(7, rt_instance->getPrimitiveDataBuffer());
    rt_ds->bindStorageBuffer(8, scene->getMaterialBuffer());
    rt_ds->bindStorageBuffer(9, scene->getAttrBuffer("NORMAL"));
    rt_ds->bindStorageBuffer(10, scene->getAttrBuffer("TEXCOORD_0"));
    scene->bindTexturesSamplers(rt_ds, 11);
    auto rt_rp = interface->createRayTracingRenderPipeline(rt_sp);
    rt_rp->setDescriptorSet(rt_ds, 0);
    rt_rp->setPushConstants(pcs);
    rt_rp->compile({.max_ray_recursion_depth = 1});

    auto vs = interface->loadShader("shader.vert", Renderer::ShaderStage::eVertex);
    auto fs = interface->loadShader("shader.frag", Renderer::ShaderStage::eFragment);

    auto graphics_sp = interface->createGraphicsShaderProgram();
    graphics_sp->attach(vs).attach(fs);
    auto image_ds = interface->createDescriptorSet();
    image_ds->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, Renderer::ShaderStage::eFragment}
    });
    image_ds->build();
    image_ds->bindTexture(0, image, sampler);
    auto graphics_rp = interface->createGraphicsRenderPipeline(renderer, graphics_sp, "main_subpass");
    graphics_rp->setDescriptorSet(image_ds, 0);
    graphics_rp->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });

    auto id = renderer->registerResourceRecreateCallback([&]() {
        image.reset();
        auto width = Renderer::renderer_config.swapchain_image_width;
        auto height = Renderer::renderer_config.swapchain_image_height;
        image = interface->createStorageImage(
            width,
            height,
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eSampled
        );
        rt_ds->bindStorageImage(2, image);
        image_ds->bindTexture(0, image, sampler);
        camera->setViewportSize(width, height);
        camera->upload();
    });

    int frame_index = 0;

    while (!global_context->window_system->shouldClose()) {
        global_context->window_system->pollEvents();

        static auto start = std::chrono::high_resolution_clock::now();
        static float last_time = 0.0f;
        auto current = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
        float delta_time = time - last_time;
        last_time = time;

        width = Renderer::renderer_config.swapchain_image_width;
        height = Renderer::renderer_config.swapchain_image_height;

        camera->update(delta_time);

        pcs->pushConstant("time", &time);

        if (camera->is_cursor_locked) {
            frame_index = 0;
        }
        pcs->pushConstant("frame_index", &frame_index);
        frame_index++;

        rt_instance->update<Material>(1, [&](uint32_t index, auto& material) {
            material = mat; 
        });

        renderer->acquireNextImage();
        renderer->bindPipeline(rt_rp);
        renderer->bindDescriptorSets(rt_rp);
        renderer->pushConstants(rt_rp);
        renderer->traceRays(rt_rp, width, height, 1);
        renderer->beginRenderPass();
        renderer->bindPipeline(graphics_rp);
        renderer->bindDescriptorSets(graphics_rp);
        renderer->setViewport(0, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height));
        renderer->setScissor(0, 0, width, height);
        renderer->draw(3, 1, 0, 0);

        imgui->newFrame();

        ImGui::Begin("Settings");

        ImGui::Text("DeltaTime: %f", ImGui::GetIO().DeltaTime);
        ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
        ImGui::Text("FrameIndex: %d", frame_index);

        bool changed = false;

        static int sample_count = 3;
        changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
        pcs->pushConstant("sample_count", &sample_count);

        static float view_depth = 1.0f;
        changed |= ImGui::SliderFloat("View Depth", &view_depth, 0.01, 5);
        pcs->pushConstant("view_depth", &view_depth);

        static float view_depth_strength = 0.01f;
        changed |= ImGui::SliderFloat("View Depth Strength", &view_depth_strength, 0, 0.5);
        pcs->pushConstant("view_depth_strength", &view_depth_strength);

        static int max_ray_recursion_depth = 6;
        changed |= ImGui::SliderInt("Max Ray Recursion Depth", &max_ray_recursion_depth, 1, 16);
        pcs->pushConstant("max_ray_recursion_depth", &max_ray_recursion_depth);

        ImGui::SeparatorText("Model Material");
        changed |= ImGui::ColorEdit3("Albedo", &mat.albedo.r);
        changed |= ImGui::SliderFloat("Roughness", &mat.roughness, 0, 1);
        changed |= ImGui::ColorEdit3("Specular Albedo", &mat.specular_albedo.r);
        changed |= ImGui::SliderFloat("Specular Probability", &mat.specular_probability, 0, 1);
        changed |= ImGui::ColorEdit3("Emissive Color", &mat.emissive_color.r);
        changed |= ImGui::SliderFloat("Emissive Intensity", &mat.emissive_intensity, 0, 1);

        if (changed) {
            frame_index = 0;
        }

        ImGui::End();

        imgui->renderFrame();

        renderer->endRenderPass();
        renderer->present();
    }
    renderer->waitIdle();

    renderer->unregisterResourceRecreateCallback(id);
    graphics_rp.reset();
    image_ds.reset();
    graphics_sp.reset();
    fs.reset();
    vs.reset();
    rt_rp.reset();
    rt_ds.reset();
    rt_sp.reset();
    closest.reset();
    triangle_rchit.reset();
    sphere_rint.reset();
    sphere_rchit.reset();
    miss.reset();
    raygen.reset();
    sampler.reset();
    image.reset();
    pcs.reset();
    camera.reset();
    rt_instance.reset();
    as.reset();
    dragon.reset();
    sphere.reset();
    scene.reset();
    imgui.reset();
    renderer.reset();
    interface.reset();

    engine->shutdownEngine();

    return 0;
}