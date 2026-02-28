#include "function/render/interface/renderer.hpp"
#include "function/render/interface/context.hpp"
#include <stb_image.h>

namespace wen::Renderer {

Image::Image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits samples, VmaMemoryUsage usage, VmaAllocationCreateFlags flags, uint32_t mip_levels, uint32_t array_layers, vk::ImageCreateFlagBits image_flags) {
    VmaAllocationCreateInfo vma_alloc_ci = {};
    vma_alloc_ci.usage = usage;
    vma_alloc_ci.flags = flags;

    vk::ImageCreateInfo image_ci = {};
    image_ci.setImageType(vk::ImageType::e2D)
        .setExtent({width, height, 1})
        .setMipLevels(mip_levels)
        .setArrayLayers(array_layers)
        .setFormat(format)
        .setTiling(vk::ImageTiling::eOptimal)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setUsage(image_usage)
        .setSamples(samples)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFlags(image_flags);

    vmaCreateImage(
        manager->vma_allocator,
        reinterpret_cast<VkImageCreateInfo*>(&image_ci),
        &vma_alloc_ci,
        reinterpret_cast<VkImage*>(&image),
        &allocation_,
        nullptr
    );
}

Image::~Image() {
    vmaDestroyImage(manager->vma_allocator, image, allocation_);
}

static void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
    vk::BufferImageCopy region;
    region.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setImageOffset({0, 0, 0})
        .setImageExtent({width, height, 1});
    
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    manager->command_pool->freeSingleUse(cmdbuf);
}

static void generateMipmaps(vk::Image image, vk::Format format, uint32_t width, uint32_t height, uint32_t mip_levels) {
    vk::FormatProperties properties = manager->device->physical_device.getFormatProperties(format);
    if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        WEN_CORE_ERROR("Texture image format does not support linear blitting")
        return;
    }

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setSubresourceRange({
            vk::ImageAspectFlagBits::eColor,
            0, 1, 0, 1
        });

    int32_t mip_width = width, mip_height = height;
    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        cmdbuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion,
            nullptr,
            nullptr,
            barrier
        );

        vk::ImageBlit blit;
        blit.setSrcOffsets({{{0, 0, 0}, {mip_width, mip_height, 1}}})
            .setSrcSubresource({
                vk::ImageAspectFlagBits::eColor,
                i - 1,
                0,
                1
            })
            .setDstOffsets({{{0, 0, 0}, {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1}}})
            .setDstSubresource({
                vk::ImageAspectFlagBits::eColor,
                i,
                0,
                1
            });
        cmdbuf.blitImage(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear
        );

        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        cmdbuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlagBits::eByRegion,
            nullptr,
            nullptr,
            barrier
        );

        if (mip_width > 1) {
            mip_width /= 2;
        }
        if (mip_height > 1) {
            mip_height /= 2;
        }
    }

    barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );
    manager->command_pool->freeSingleUse(cmdbuf);
}

DataTexture::DataTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels) {
    uint32_t size = width * height * 4;
    Buffer staging_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    );
    memcpy(staging_buffer.map(), data, size);
    staging_buffer.unmap();

    if (mip_levels == 0) {
        mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
    }
    mip_levels_ = mip_levels;

    image_ = std::make_unique<Image>(
        width, height,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        vk::SampleCountFlagBits::e1,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        mip_levels
    );

    transitionImageLayout(
        image_->image,
        vk::ImageAspectFlagBits::eColor,
        mip_levels,
        {
            vk::ImageLayout::eUndefined,
            vk::AccessFlagBits::eNone,
            vk::PipelineStageFlagBits::eTopOfPipe,
        },
        {
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits::eTransferWrite,
            vk::PipelineStageFlagBits::eTransfer,
        }
    );

    copyBufferToImage(staging_buffer.buffer, image_->image, width, height);
    generateMipmaps(image_->image, vk::Format::eR8G8B8A8Srgb, width, height, mip_levels);

    image_view_ = createImageView(
        image_->image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        mip_levels
    );
}

DataTexture::~DataTexture() {
    manager->device->device.destroyImageView(image_view_);
    image_.reset();
}

ImageTexture::ImageTexture(const std::string& filename, uint32_t mip_levels) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        WEN_CORE_ERROR("Failed to load image: {}", filename)
        return;
    }
    texture_ = std::make_unique<DataTexture>(pixels, width, height, mip_levels);
    stbi_image_free(pixels);
}

ImageTexture::~ImageTexture() {
    texture_.reset();
}

StorageImage::StorageImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage) {
    image_ = std::make_unique<Image>(
        width, height,
        format,
        usage | vk::ImageUsageFlagBits::eStorage,
        vk::SampleCountFlagBits::e1,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        1
    );
    transitionImageLayout(
        image_->image,
        vk::ImageAspectFlagBits::eColor,
        1,
        {
            vk::ImageLayout::eUndefined,
            vk::AccessFlagBits::eNone,
            vk::PipelineStageFlagBits::eTopOfPipe
        },
        {
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits::eMemoryRead,
            vk::PipelineStageFlagBits::eAllGraphics
        }
    );
    image_view_ = createImageView(
        image_->image,
        format,
        vk::ImageAspectFlagBits::eColor,
        1
    );
}

StorageImage::~StorageImage() {
    manager->device->device.destroyImageView(image_view_);
    image_.reset();
};

Sampler::Sampler(const SamplerOptions& options) {
    vk::SamplerCreateInfo sampler_ci;
    sampler_ci.setMagFilter(options.mag_filter)
        .setMinFilter(options.min_filter)
        .setAddressModeU(options.address_mode_u)
        .setAddressModeV(options.address_mode_v)
        .setAddressModeW(options.address_mode_w)
        .setAnisotropyEnable(options.max_anisotropy > 1)
        .setMaxAnisotropy(options.max_anisotropy)
        .setBorderColor(options.border_color)
        .setUnnormalizedCoordinates(false)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(options.mipmap_mode)
        .setMinLod(0.0f)
        .setMaxLod(static_cast<float>(options.mip_levels))
        .setMipLodBias(0.0f);
    sampler = manager->device->device.createSampler(sampler_ci);
}

Sampler::~Sampler() {
    manager->device->device.destroySampler(sampler);
}

Attachment::Attachment(vk::AttachmentDescription description, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) {
    image = std::make_unique<Image>(
        renderer_config.swapchain_image_width,
        renderer_config.swapchain_image_height,
        description.format,
        usage,
        description.samples,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    image_view = createImageView(image->image, description.format, aspect, 1);
}

Attachment::~Attachment() {
    image.reset();
    manager->device->device.destroyImageView(image_view);
}

Framebuffer::Framebuffer(const Renderer& renderer, const std::vector<vk::ImageView>& image_views) {
    vk::FramebufferCreateInfo framebuffer_ci;
    framebuffer_ci.setRenderPass(renderer.render_pass->render_pass)
        .setWidth(renderer_config.swapchain_image_width)
        .setHeight(renderer_config.swapchain_image_height)
        .setLayers(1)
        .setAttachments(image_views);
    framebuffer_ = manager->device->device.createFramebuffer(framebuffer_ci);
}

Framebuffer::~Framebuffer() {
    manager->device->device.destroyFramebuffer(framebuffer_);
}

FramebufferSet::FramebufferSet(const Renderer& renderer) {
    uint32_t count = renderer.render_pass->attachments.size();
    uint32_t final_count = renderer.render_pass->final_attachments.size();

    attachments.resize(final_count);
    for (auto [name, index] : renderer.render_pass->getAttachmentIndices()) {
        auto attachment = renderer.render_pass->attachments[index];
        attachments[index] = new Attachment(
            attachment.attachment,
            attachment.usage,
            attachment.aspect
        );
        if (index == 0 || index == 1) continue;
        if (renderer_config.msaa()) {
            auto resolve_attachment = renderer.render_pass->resolve_attachments[index - 1];
            attachments[renderer.render_pass->getAttachmentIndex(name, true)] = new Attachment(
                resolve_attachment.attachment,
                attachment.usage,
                attachment.aspect
            );
        }
    }

    std::vector<vk::ImageView> image_views(final_count);
    for (uint32_t i = 0; i < final_count; i++) {
        if (renderer_config.msaa() && i == count) continue;
        image_views[i] = attachments[i]->image_view;
    }
    for (auto image_view : manager->swapchain->image_views) {
        image_views[renderer_config.msaa() ? count : 0] = image_view;
        framebuffers.push_back(std::make_unique<Framebuffer>(renderer, image_views));
    }
}

FramebufferSet::~FramebufferSet() {
    for (auto attachment : attachments) {
        delete attachment;
    }
    attachments.clear();
    framebuffers.clear();
}

}  // namespace wen::Renderer