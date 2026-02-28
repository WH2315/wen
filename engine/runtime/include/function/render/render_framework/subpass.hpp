#pragma once

#include "function/render/interface/interface.hpp"
#include "function/render/render_framework/resource.hpp"

namespace wen {

class Subpass {
public:
    Subpass(const std::string& name, bool only_compute = false) : name_(name), only_compute_(only_compute) {}
    virtual ~Subpass() = default;

    virtual void addAttachment(Renderer::RenderPass& render_pass) {}
    virtual void setAttachment(Renderer::RenderSubpass& render_subpass) {}
    virtual void createRenderResource(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {}
    virtual void executePreRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {}
    virtual void executeRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {}
    virtual void executePostRenderPass(std::shared_ptr<Renderer::Renderer> renderer, Resource& resource) {}

    auto getName() const { return name_; }
    auto isOnlyCompute() const { return only_compute_; }

protected:
    std::string name_;
    bool only_compute_;
};

}  // namespace wen