#include "function/render/mesh/mesh_instance_pool.hpp"

namespace wen {

MeshInstancePool::MeshInstancePool(uint32_t max_mesh_instance_count) {
    // 网格实例数据存储在一个连续的缓冲区中，方便一次性上传到GPU
    mesh_instance_buffer = std::make_shared<Renderer::Buffer>(
        sizeof(MeshInstance) * max_mesh_instance_count,
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    // 将缓冲区映射到CPU地址空间，获取指向网格实例数据的指针
    mesh_instance_buffer_ptr = static_cast<MeshInstance*>(mesh_instance_buffer->map());
}

void MeshInstancePool::createMeshInstance(const MeshInstance& mesh_instance, GameObjectUUID uuid) {
    // 将新的网格实例数据写入缓冲区，并更新相关的映射关系
    *mesh_instance_buffer_ptr = mesh_instance;
    mesh_instance_buffer_ptr++;
    current_instance_count++;
    game_object_uuid_to_mesh_instance_index_map.insert({uuid, current_instance_count - 1});
    mesh_instance_index_to_game_object_uuid_map.insert({current_instance_count - 1, uuid});
}

MeshInstance* MeshInstancePool::getMeshInstancePtr(GameObjectUUID uuid) {
    return static_cast<MeshInstance*>(mesh_instance_buffer->map()) +
           game_object_uuid_to_mesh_instance_index_map.at(uuid);
}

void MeshInstancePool::clear() {
    mesh_instance_buffer_ptr = static_cast<MeshInstance*>(mesh_instance_buffer->map());
    current_instance_count = 0;
    memset(mesh_instance_buffer_ptr, 0, mesh_instance_buffer->size);
}

}  // namespace wen