#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace wen::Renderer {

class Image {
public:
    Image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits samples, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, uint32_t mip_levels = 1, uint32_t array_layers = 1, vk::ImageCreateFlagBits image_flags = {});
    ~Image();

    vk::Image image;

private:
    VmaAllocation allocation_;
};

class SpecificTexture {
public:
    SpecificTexture() = default;
    virtual ~SpecificTexture() = default;
    virtual vk::ImageLayout getImageLayout() = 0;
    virtual vk::ImageView getImageView() = 0;
    virtual uint32_t getMipLevels() = 0;
    // 加载是否成功(资源创建失败时返回 false,便于上层兜底)。
    virtual bool isValid() const { return true; }
};

class DataTexture : public SpecificTexture {
public:
    DataTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels);
    ~DataTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return mip_levels_; }
    bool isValid() const override { return image_ != nullptr; }

private:
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
    uint32_t mip_levels_;
};

class ImageTexture : public SpecificTexture {
public:
    ImageTexture(const std::string& filename, uint32_t mip_levels);
    ~ImageTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return texture_->getImageView(); }
    uint32_t getMipLevels() override { return texture_->getMipLevels(); }
    bool isValid() const override { return texture_ != nullptr; }

private:
    std::unique_ptr<DataTexture> texture_;
};

class StorageImage : public SpecificTexture {
public:
    StorageImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage);
    ~StorageImage() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eGeneral; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return 1; }
    auto getImage() { return image_->image; }

private:
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
};

// 立方体贴图:cube-compatible 的 6 层图像,含采样用的 cube 视图,
// 以及每个 mip 一个 2D array 视图(供计算着色器逐层写入)。
class CubeTexture : public SpecificTexture {
public:
    CubeTexture(uint32_t size, uint32_t mip_levels, vk::Format format, vk::ImageUsageFlags extra_usage);
    ~CubeTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return cube_view_; }
    uint32_t getMipLevels() override { return mip_levels_; }
    bool isValid() const override { return image_ != nullptr; }

    auto getImage() { return image_->image; }
    vk::ImageView getArrayView(uint32_t mip) { return array_views_[mip]; }

private:
    std::unique_ptr<Image> image_;
    vk::ImageView cube_view_;
    std::vector<vk::ImageView> array_views_;
    uint32_t mip_levels_;
};

class DepthImage : public SpecificTexture {
public:
    DepthImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, uint32_t mip_levels);
    ~DepthImage() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eGeneral; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return mip_levels_; }

    auto getImage() { return image_->image; }
    auto getMipmapViews() { return mipmap_views_; }

private:
    uint32_t mip_levels_;
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
    std::vector<vk::ImageView> mipmap_views_;
};

struct SamplerOptions {
    vk::Filter mag_filter = vk::Filter::eLinear;
    vk::Filter min_filter = vk::Filter::eLinear;
    vk::SamplerAddressMode address_mode_u = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode address_mode_v = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode address_mode_w = vk::SamplerAddressMode::eRepeat;
    uint32_t max_anisotropy = 1;
    vk::BorderColor border_color = vk::BorderColor::eIntOpaqueBlack;
    vk::SamplerMipmapMode mipmap_mode = vk::SamplerMipmapMode::eLinear;
    uint32_t mip_levels = 1;
};

class Sampler final {
public:
    Sampler(const SamplerOptions& options);
    ~Sampler();

    vk::Sampler sampler;
};

struct Attachment {
    Attachment(vk::AttachmentDescription description, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);
    ~Attachment();

    std::unique_ptr<Image> image;
    vk::ImageView image_view;
};

class Renderer;
class Framebuffer {
    friend class Renderer;

public:
    Framebuffer(const Renderer& renderer, const std::vector<vk::ImageView>& image_views);
    ~Framebuffer();

private:
    vk::Framebuffer framebuffer_;
};

struct FramebufferSet {
    FramebufferSet(const Renderer& renderer);
    ~FramebufferSet();

    std::vector<Attachment*> attachments;
    std::vector<std::unique_ptr<Framebuffer>> framebuffers;
};

}  // namespace wen::Renderer