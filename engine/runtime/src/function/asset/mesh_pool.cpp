#include "function/asset/mesh_pool.hpp"
#include "engine/global_context.hpp"

namespace wen {

MeshPool::MeshPool(uint64_t vertex_memory_size, uint64_t index_memory_size, uint32_t max_mesh_descriptor_count, uint32_t max_primitive_descriptor_count) {
    auto interface = global_context->render_system->getInterface();
    current_vertex_count = 0;
    current_index_count = 0;
    max_vertex_count = vertex_memory_size / sizeof(glm::vec3);
    max_index_count = index_memory_size / sizeof(uint32_t);

    position_buffer = interface->createVertexBuffer(sizeof(glm::vec3), max_vertex_count, vk::BufferUsageFlagBits::eStorageBuffer);
    normal_buffer = interface->createVertexBuffer(sizeof(glm::vec3), max_vertex_count, vk::BufferUsageFlagBits::eStorageBuffer);
    tex_coord_buffer = interface->createVertexBuffer(sizeof(glm::vec2), max_vertex_count, vk::BufferUsageFlagBits::eStorageBuffer);
    color_buffer = interface->createVertexBuffer(sizeof(glm::vec3), max_vertex_count, vk::BufferUsageFlagBits::eStorageBuffer);
    index_buffer = interface->createIndexBuffer(Renderer::IndexType::eUint32, max_index_count, vk::BufferUsageFlagBits::eStorageBuffer);

    current_primitive_descriptor_count = 0;
    primitive_descriptor_buffer = std::make_shared<Renderer::Buffer>(
        sizeof(PrimitiveDescriptor) * max_primitive_descriptor_count,
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    primitive_descriptor_buffer_ptr = static_cast<PrimitiveDescriptor*>(primitive_descriptor_buffer->map());

    current_mesh_descriptor_count = 0;
    mesh_descriptor_buffer = std::make_shared<Renderer::Buffer>(
        sizeof(MeshDescriptor) * max_mesh_descriptor_count,
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    mesh_descriptor_buffer_ptr = static_cast<MeshDescriptor*>(mesh_descriptor_buffer->map());
}

MeshPool::~MeshPool() {
    position_buffer.reset();
    normal_buffer.reset();
    tex_coord_buffer.reset();
    color_buffer.reset();
    index_buffer.reset();
}

MeshID MeshPool::uploadMeshData(const MeshData& mesh_data) {
    size_t vertex_count = 0, index_count = 0;
    for (auto& primitive : mesh_data.lods) {
        vertex_count += primitive.positions.size();
        index_count += primitive.indices.size();
    }
    if (current_vertex_count + vertex_count > max_vertex_count) {
        WEN_CORE_ERROR("MeshPool vertex memory overflow. Current: {}, Required: {}, Max: {}", current_vertex_count, vertex_count, max_vertex_count)
        return -1;
    }
    if (current_index_count + index_count > max_index_count) {
        WEN_CORE_ERROR("MeshPool index memory overflow. Current: {}, Required: {}, Max: {}", current_index_count, index_count, max_index_count)
        return -1;
    }

    mesh_descriptor_buffer_ptr->aabb_min = {std::numeric_limits<float>::max(),
                                            std::numeric_limits<float>::max(),
                                            std::numeric_limits<float>::max()};
    mesh_descriptor_buffer_ptr->aabb_max = {std::numeric_limits<float>::min(),
                                            std::numeric_limits<float>::min(),
                                            std::numeric_limits<float>::min()};
    mesh_descriptor_buffer_ptr->radius = 0;
    size_t lod_index = 0;
    for (auto& primitive : mesh_data.lods) {
        position_buffer->setData(primitive.positions, current_vertex_count);
        normal_buffer->setData(primitive.normals, current_vertex_count);
        tex_coord_buffer->setData(primitive.tex_coords, current_vertex_count);
        color_buffer->setData(primitive.colors, current_vertex_count);
        index_buffer->setData(primitive.indices, current_index_count);

        auto primitive_id = current_primitive_descriptor_count;
        current_primitive_descriptor_count++;

        primitive_descriptor_buffer_ptr->vertex_offset = current_vertex_count;
        primitive_descriptor_buffer_ptr->first_index = current_index_count;
        primitive_descriptor_buffer_ptr->index_count = primitive.indices.size();
        primitive_descriptor_buffer_ptr++;
        current_vertex_count += primitive.positions.size();
        current_index_count += primitive.indices.size();
        mesh_descriptor_buffer_ptr->lods[lod_index] = primitive_id;
        lod_index++;

        for (auto& position : primitive.positions) {
            mesh_descriptor_buffer_ptr->aabb_min = glm::min(mesh_descriptor_buffer_ptr->aabb_min, position);
            mesh_descriptor_buffer_ptr->aabb_max = glm::max(mesh_descriptor_buffer_ptr->aabb_max, position);
            mesh_descriptor_buffer_ptr->radius = std::max(mesh_descriptor_buffer_ptr->radius, glm::dot(position, position));
        }
    }
    mesh_descriptor_buffer_ptr->lod_count = mesh_data.lods.size();
    mesh_descriptor_buffer_ptr->radius = std::sqrt(mesh_descriptor_buffer_ptr->radius);
    mesh_descriptor_buffer_ptr++;
    current_mesh_descriptor_count++;

    return current_mesh_descriptor_count - 1;
}

}  // namespace wen