#pragma once

#include "function/render/interface/basic/enums.hpp"

namespace wen::Renderer {

class RenderPass;
class RenderSubpass {
public:
    RenderSubpass(const std::string& name, RenderPass& render_pass);
    ~RenderSubpass();

    void setOutputAttachment(const std::string& name, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);
    void setDepthAttachment(const std::string& name, vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);
    void setInputAttachment(const std::string& name, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::SubpassDescription build();

public:
    std::string name;
    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;

private:
    RenderPass& render_pass_;
    std::vector<vk::AttachmentReference> output_attachments_;
    std::optional<vk::AttachmentReference> depth_attachment_;
    std::vector<vk::AttachmentReference> resolve_attachments_;
    std::vector<vk::AttachmentReference> input_attachments_;

private:
    vk::AttachmentReference createAttachmentReference(const std::string& name, vk::ImageLayout layout, bool read, const char* caller = "unknown");
};

struct AttachmentInfo {
    std::string name;
    vk::AttachmentDescription attachment = {};
    vk::ImageUsageFlags usage = {};
    vk::ImageAspectFlags aspect = {};
    vk::ClearValue clear_color = {};
};

struct ResolveAttachmentInfo {
    std::string name;
    vk::AttachmentDescription attachment = {};
    uint32_t offset = 0;
};

class RenderPass {
public:
    RenderPass(bool auto_load);
    ~RenderPass();

    void addAttachment(const std::string& name, AttachmentType type);
    RenderSubpass& addSubpass(const std::string& name);
    void addSubpassDependency(const std::string& src, const std::string& dst, std::array<vk::PipelineStageFlags, 2> stage, std::array<vk::AccessFlags, 2> access);

    void build();
    void update();

    uint32_t getAttachmentIndex(const std::string& name, bool read) const;
    uint32_t getSubpassIndex(const std::string& name) const;
    auto getAttachmentIndices() const { return attachment_indices_; }

public:
    std::vector<AttachmentInfo> attachments;
    std::vector<ResolveAttachmentInfo> resolve_attachments;
    std::vector<std::unique_ptr<RenderSubpass>> subpasses;

    vk::RenderPass render_pass;
    std::vector<vk::AttachmentDescription> final_attachments;
    std::vector<vk::SubpassDescription> final_subpasses;
    std::vector<vk::SubpassDependency> final_dependencies;

private:
    std::unordered_map<std::string, uint32_t> attachment_indices_;
    std::unordered_map<std::string, uint32_t> subpass_indices_;
};

}  // namespace wen::Renderer