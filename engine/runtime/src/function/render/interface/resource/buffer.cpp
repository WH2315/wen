#include "function/render/interface/resource/buffer.hpp"
#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/context.hpp"

namespace wen::Renderer {

Buffer::Buffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage usage, VmaAllocationCreateFlags flags) 
    : size(size), data(nullptr), mapped_(false) {
    VmaAllocationCreateInfo vma_alloc_ci = {};
    vma_alloc_ci.usage = usage;
    vma_alloc_ci.flags = flags;

    vk::BufferCreateInfo buffer_ci = {};
    buffer_ci.setSize(size)
        .setUsage(buffer_usage);
    
    vmaCreateBuffer(
        manager->vma_allocator,
        reinterpret_cast<VkBufferCreateInfo*>(&buffer_ci),
        &vma_alloc_ci,
        reinterpret_cast<VkBuffer*>(&buffer),
        &allocation_,
        nullptr
    );
}

void* Buffer::map() {
    if (mapped_) {
        return data;
    }
    vmaMapMemory(manager->vma_allocator, allocation_, &data);
    mapped_ = true;
    return data;
}

void Buffer::unmap() {
    if (!mapped_) {
        return;
    }
    vmaUnmapMemory(manager->vma_allocator, allocation_);
    data = nullptr;
    mapped_ = false;
}

Buffer::~Buffer() {
    unmap();
    vmaDestroyBuffer(manager->vma_allocator, buffer, allocation_);
}

VertexBuffer::VertexBuffer(uint32_t size, uint32_t count) {
    staging_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(size) * count,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY,
        0
    );
    buffer_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(size) * count,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
}

VertexBuffer::~VertexBuffer() {
    staging_.reset();
    buffer_.reset();
}

void* VertexBuffer::map() {
    return staging_->map();
}

void VertexBuffer::flush() {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy regions;
    regions.setSize(staging_->size).setSrcOffset(0).setDstOffset(0);
    cmdbuf.copyBuffer(staging_->buffer, buffer_->buffer, regions);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void VertexBuffer::unmap() {
    staging_->unmap();
}

IndexBuffer::IndexBuffer(IndexType index_type, uint32_t count) {
    index_type_ = convert<vk::IndexType>(index_type);
    staging_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(count) * convert<uint32_t>(index_type),
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY,
        0
    );
    buffer_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(count) * convert<uint32_t>(index_type),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
}

IndexBuffer::~IndexBuffer() {
    staging_.reset();
    buffer_.reset();
}

void* IndexBuffer::map() {
    return staging_->map();
}

void IndexBuffer::flush() {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy regions;
    regions.setSize(staging_->size).setSrcOffset(0).setDstOffset(0);
    cmdbuf.copyBuffer(staging_->buffer, buffer_->buffer, regions);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void IndexBuffer::unmap() {
    staging_->unmap();
}

UniformBuffer::UniformBuffer(uint64_t size) {
    buffer_ = std::make_unique<Buffer>(
        size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    buffer_->map();
}

UniformBuffer::~UniformBuffer() {
    buffer_.reset();
}

StorageBuffer::StorageBuffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags) {
    buffer_ = std::make_unique<Buffer>(
        size,
        buffer_usage | vk::BufferUsageFlagBits::eStorageBuffer,
        memory_usage,
        allocation_flags
    );
}

void* StorageBuffer::map() {
    return buffer_->map();
}

void StorageBuffer::flush(vk::DeviceSize size, const vk::Buffer& buffer) {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy regions;
    regions.setSize(size).setSrcOffset(0).setDstOffset(0);
    cmdbuf.copyBuffer(buffer_->buffer, buffer, regions);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void StorageBuffer::unmap() {
    buffer_->unmap();
}

StorageBuffer::~StorageBuffer() {
    buffer_.reset();
}

}  // namespace wen::Renderer