#pragma once

#include "function/asset/mesh/mesh.hpp"
#include "function/render/interface/resource/buffer.hpp"

namespace wen {

class MeshPool {
public:
    MeshPool(uint64_t vertex_memory_size, uint64_t index_memory_size, uint32_t max_mesh_descriptor_count, uint32_t max_primitive_descriptor_count);
    ~MeshPool();

    MeshID uploadMeshData(const MeshData& mesh_data);
    auto getMeshCount() const { return current_mesh_descriptor_count; }
    auto getPrimitiveCount() const { return current_primitive_descriptor_count; }

public:
    uint32_t current_vertex_count;
    uint32_t current_index_count;
    uint32_t max_vertex_count;
    uint32_t max_index_count;

    std::shared_ptr<Renderer::VertexBuffer> position_buffer;
    std::shared_ptr<Renderer::VertexBuffer> normal_buffer;
    std::shared_ptr<Renderer::VertexBuffer> tex_coord_buffer;
    std::shared_ptr<Renderer::VertexBuffer> color_buffer;
    std::shared_ptr<Renderer::IndexBuffer> index_buffer;

    std::shared_ptr<Renderer::Buffer> primitive_descriptor_buffer;
    PrimitiveDescriptor* primitive_descriptor_buffer_ptr;
    uint32_t current_primitive_descriptor_count;

    std::shared_ptr<Renderer::Buffer> mesh_descriptor_buffer;
    MeshDescriptor* mesh_descriptor_buffer_ptr;
    uint32_t current_mesh_descriptor_count;
};

}  // namespace wen