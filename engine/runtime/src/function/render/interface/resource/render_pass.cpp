#include "function/render/interface/resource/render_pass.hpp"
#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/context.hpp"
#include "core/base/macro.hpp"

namespace wen::Renderer {

RenderSubpass::RenderSubpass(const std::string& name, RenderPass& render_pass)
    : name(name), render_pass_(render_pass) {}

RenderSubpass::~RenderSubpass() {}

void RenderSubpass::setOutputAttachment(const std::string& name, vk::ImageLayout layout) {
    output_attachments_.push_back(createAttachmentReference(name, layout, false, "setOutputAttachment"));
    color_blend_attachments.push_back({
        false,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    });
    if (renderer_config.msaa()) {
        resolve_attachments_.push_back(createAttachmentReference(name, layout, true, "setOutputAttachment(resolve)"));
    }
}

void RenderSubpass::setDepthAttachment(const std::string& name, vk::ImageLayout layout) {
    depth_attachment_ = createAttachmentReference(name, layout, false, "setDepthAttachment");
}

void RenderSubpass::setInputAttachment(const std::string& name, vk::ImageLayout layout) {
    input_attachments_.push_back(createAttachmentReference(name, layout, true, "setInputAttachment"));
}

vk::SubpassDescription RenderSubpass::build() {
    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(output_attachments_.size())
        .setColorAttachments(output_attachments_);
    if (depth_attachment_.has_value()) {
        subpass.setPDepthStencilAttachment(&depth_attachment_.value());
    } 
    if (renderer_config.msaa()) {
        subpass.setResolveAttachments(resolve_attachments_);
    }
    subpass.setInputAttachmentCount(input_attachments_.size())
        .setInputAttachments(input_attachments_);
    return subpass;
}

vk::AttachmentReference RenderSubpass::createAttachmentReference(const std::string& name, vk::ImageLayout layout, bool read, const char* caller) {
    uint32_t attachment = render_pass_.getAttachmentIndex(name, read);
    std::string label = (!renderer_config.msaa() || !read)
                            ? "attachment"
                            : "resolve_attachment";
    WEN_CORE_DEBUG("\"{}\": {} -> {}_index: {}, name: {}", this->name,
                   caller ? caller : "unknown", label, attachment, name)
    vk::AttachmentReference reference = {};
    reference.setAttachment(attachment).setLayout(layout);
    return reference;
}

RenderPass::RenderPass(bool auto_load) {
    if (auto_load) {
        addAttachment(SWAPCHAIN_IMAGE_ATTACHMENT, AttachmentType::eColor);
        addAttachment(DEPTH_ATTACHMENT, AttachmentType::eDepth);
    }
}

RenderPass::~RenderPass() {
    attachments.clear();
    for (auto& subpass : subpasses) {
        subpass.reset();
    }
    subpasses.clear();

    manager->device->device.destroyRenderPass(render_pass);
    final_attachments.clear();
    final_subpasses.clear();
    final_dependencies.clear();
}

void RenderPass::addAttachment(const std::string& name, AttachmentType type) {
    attachment_indices_.insert(std::make_pair(name, attachments.size()));
    auto& attachment = attachments.emplace_back();

    attachment.name = name;
    attachment.attachment
        .setSamples(renderer_config.msaa_samples)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined); 
    
    switch (type) {
        case AttachmentType::eColor:
            attachment.attachment
                .setFormat(manager->swapchain->format.format)
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
            attachment.usage = vk::ImageUsageFlagBits::eColorAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            if (renderer_config.msaa()) {
                attachment.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
                auto& resolve_attachment = resolve_attachments.emplace_back();
                resolve_attachment.name = name;
                resolve_attachment.attachment = attachment.attachment;
                resolve_attachment.attachment.setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eDontCare);
                resolve_attachment.offset = resolve_attachments.size() - 1;
            }
            break;
        case AttachmentType::eDepth:
            attachment.attachment
                .setFormat(findDepthFormat())
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setStencilLoadOp(vk::AttachmentLoadOp::eClear);
            attachment.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
            attachment.aspect = vk::ImageAspectFlagBits::eDepth;
            if (attachment.attachment.format == vk::Format::eD32SfloatS8Uint || attachment.attachment.format == vk::Format::eD24UnormS8Uint) {
                attachment.aspect |= vk::ImageAspectFlagBits::eStencil;
            }
            attachment.clear_color = vk::ClearDepthStencilValue(1.0f, 0);
            break;
        case AttachmentType::eStencil:
            attachment.attachment
                .setFormat(vk::Format::eD24UnormS8Uint)
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setStencilLoadOp(vk::AttachmentLoadOp::eClear);
            attachment.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
            attachment.aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            attachment.clear_color = vk::ClearDepthStencilValue(1.0f, 0);
            break;
        case AttachmentType::eRGBA8Snorm:
            attachment.attachment
                .setFormat(vk::Format::eR8G8B8A8Snorm)
                .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            attachment.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            if (renderer_config.msaa()) {
                auto& resolve_attachment = resolve_attachments.emplace_back();
                resolve_attachment.name = name;
                resolve_attachment.attachment = attachment.attachment;
                resolve_attachment.attachment.setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eDontCare);
                resolve_attachment.offset = resolve_attachments.size() - 1;
                attachment.attachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
            }
            break;
        case AttachmentType::eRGBA32Sfloat:
            attachment.attachment
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            attachment.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            if (renderer_config.msaa()) {
                auto& resolve_attachment = resolve_attachments.emplace_back();
                resolve_attachment.name = name;
                resolve_attachment.attachment = attachment.attachment;
                resolve_attachment.attachment.setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eDontCare);
                resolve_attachment.offset = resolve_attachments.size() - 1;
                attachment.attachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
            }
            break;
    }

    // Ensure the swapchain color attachment resolves to PRESENT so queue present is valid.
    if (name == SWAPCHAIN_IMAGE_ATTACHMENT) {
        if (renderer_config.msaa()) {
            // With MSAA we store swapchain resolve at index 0.
            resolve_attachments.back().attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        } else {
            attachments.back().attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        }
    }
}

RenderSubpass& RenderPass::addSubpass(const std::string& name) {
    subpass_indices_.insert(std::make_pair(name, subpasses.size())); 
    subpasses.push_back(std::make_unique<RenderSubpass>(name, *this));
    return *subpasses.back();
}

void RenderPass::addSubpassDependency(const std::string& src, const std::string& dst, std::array<vk::PipelineStageFlags, 2> stage, std::array<vk::AccessFlags, 2> access) {
    final_dependencies.push_back({
        src == EXTERNAL_SUBPASS ? VK_SUBPASS_EXTERNAL : getSubpassIndex(src),
        getSubpassIndex(dst),
        stage[0],
        stage[1],
        access[0],
        access[1]
    });
}

void RenderPass::build() {
    vk::RenderPassCreateInfo render_pass_ci;

    final_attachments.clear();
    final_attachments.reserve(attachments.size() + resolve_attachments.size());
    for (const auto& attachment : attachments) {
        final_attachments.push_back(attachment.attachment);
    }
    for (const auto& attachment : resolve_attachments) {
        final_attachments.push_back(attachment.attachment);
    }

    final_subpasses.clear();
    final_subpasses.reserve(subpasses.size());
    for (const auto& subpass : subpasses) {
        final_subpasses.push_back(subpass->build());
    }

    render_pass_ci.setAttachmentCount(final_attachments.size())
        .setAttachments(final_attachments)
        .setSubpassCount(final_subpasses.size())
        .setSubpasses(final_subpasses)
        .setDependencyCount(final_dependencies.size())
        .setDependencies(final_dependencies);
    
    render_pass = manager->device->device.createRenderPass(render_pass_ci);
}

void RenderPass::update() {
    manager->device->device.destroyRenderPass(render_pass);
    build();
}

uint32_t RenderPass::getAttachmentIndex(const std::string& name, bool read) const {
    auto it = attachment_indices_.find(name);
    if (it == attachment_indices_.end()) {
        WEN_CORE_ERROR("Attachment \"{}\" not found", name)
    }
    if (read && attachments[it->second].attachment.samples != vk::SampleCountFlagBits::e1) {
        auto index = it->second == 0 ? 0 : it->second - 1;
        return attachments.size() + resolve_attachments[index].offset;
    }
    return it->second;
}

uint32_t RenderPass::getSubpassIndex(const std::string& name) const {
    auto it = subpass_indices_.find(name);
    if (it == subpass_indices_.end()) {
        WEN_CORE_ERROR("Subpass \"{}\" not found", name)
    }
    return it->second;
}

}  // namespace wen::Renderer