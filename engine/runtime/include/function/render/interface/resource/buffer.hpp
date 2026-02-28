#pragma once

#include "function/render/interface/basic/enums.hpp"
#include <vk_mem_alloc.h>

namespace wen::Renderer {

class Buffer {
public:
    Buffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage usage, VmaAllocationCreateFlags flags);
    ~Buffer();

    void* map();
    void unmap();

public:
    vk::Buffer buffer;
    uint64_t size;
    void* data;

private:
    VmaAllocation allocation_;
    bool mapped_;
};

class SpecificBuffer {
public:
    SpecificBuffer() = default;
    virtual ~SpecificBuffer() = default;
    virtual vk::Buffer getBuffer() = 0;
    virtual uint64_t getSize() = 0;
    virtual void* getData() = 0;
};

class VertexBuffer : public SpecificBuffer {
public:
    VertexBuffer(uint32_t size, uint32_t count);
    ~VertexBuffer() override;

    void* map();
    void flush();
    void unmap();

    template <class Type>
    uint32_t setData(const std::vector<Type>& data, uint32_t offset = 0) {
        auto* ptr = static_cast<uint8_t*>(staging_->map());
        memcpy(ptr + (offset * sizeof(Type)), data.data(), data.size() * sizeof(Type));
        flush();
        staging_->unmap();
        return offset + data.size();
    }

    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; }

private:
    std::unique_ptr<Buffer> staging_;
    std::unique_ptr<Buffer> buffer_;
};

class IndexBuffer : public SpecificBuffer {
public:
    IndexBuffer(IndexType index_type, uint32_t count);
    ~IndexBuffer() override;

    void* map();
    void flush();
    void unmap();

    template <class Type>
    uint32_t setData(const std::vector<Type>& data, uint32_t offset = 0) {
        auto* ptr = static_cast<uint8_t*>(map());
        memcpy(ptr + (offset * sizeof(Type)), data.data(), data.size() * sizeof(Type));
        flush();
        unmap();
        return offset + data.size();
    }

    vk::IndexType getIndexType() { return index_type_; }
    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; }

private:
    vk::IndexType index_type_;
    std::unique_ptr<Buffer> staging_;
    std::unique_ptr<Buffer> buffer_;
};

class UniformBuffer : public SpecificBuffer {
public:
    UniformBuffer(uint64_t size);
    ~UniformBuffer() override;

    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; };

private:
    std::unique_ptr<Buffer> buffer_;
};

class StorageBuffer : public SpecificBuffer {
public:
    StorageBuffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags);
    ~StorageBuffer() override;

    void* map();
    void flush(vk::DeviceSize size, const vk::Buffer& buffer);
    void unmap();

    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; }

private:
    std::unique_ptr<Buffer> buffer_;
};

}  // namespace wen::Renderer