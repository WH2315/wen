#include "function/render/mesh/mesh_instance_pool.hpp"

namespace wen {

MeshInstancePool::MeshInstancePool(uint32_t max_mesh_instance_count) {
    current_instance_count = 0;
    // 网格实例数据存储在一个连续的缓冲区中，方便一次性上传到GPU
    mesh_instance_buffer = std::make_shared<Renderer::StorageBuffer>(
        sizeof(MeshInstance) * max_mesh_instance_count,
        vk::BufferUsageFlags{},
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

void MeshInstancePool::removeMeshInstance(GameObjectUUID uuid) {
    auto iter = game_object_uuid_to_mesh_instance_index_map.find(uuid);
    if (iter == game_object_uuid_to_mesh_instance_index_map.end()) {
        return;
    }
    uint32_t index = iter->second;
    uint32_t last = current_instance_count - 1;
    auto* base = static_cast<MeshInstance*>(mesh_instance_buffer->map());

    if (index != last) {
        // 把末尾实例搬到被删位置,并修正它的索引映射
        base[index] = base[last];
        auto moved_uuid = mesh_instance_index_to_game_object_uuid_map.at(last);
        game_object_uuid_to_mesh_instance_index_map[moved_uuid] = index;
        mesh_instance_index_to_game_object_uuid_map[index] = moved_uuid;
    }

    game_object_uuid_to_mesh_instance_index_map.erase(uuid);
    mesh_instance_index_to_game_object_uuid_map.erase(last);
    current_instance_count--;
    // 保持写入指针指向下一个空位
    mesh_instance_buffer_ptr = base + current_instance_count;
}

MeshInstance* MeshInstancePool::getMeshInstancePtr(GameObjectUUID uuid) {
    return static_cast<MeshInstance*>(mesh_instance_buffer->map()) +
           game_object_uuid_to_mesh_instance_index_map.at(uuid);
}

void MeshInstancePool::clear() {
    mesh_instance_buffer_ptr = static_cast<MeshInstance*>(mesh_instance_buffer->map());
    current_instance_count = 0;
    memset(mesh_instance_buffer_ptr, 0, mesh_instance_buffer->getSize());
}

}  // namespace wen