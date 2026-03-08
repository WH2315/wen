#pragma once

#include "function/render/mesh/mesh_instance.hpp"
#include "function/framework/uuid_manager.hpp"
#include "function/render/interface/resource/buffer.hpp"

namespace wen {

// 网格实例池，管理所有的网格实例
class MeshInstancePool {
public:
    MeshInstancePool(uint32_t max_mesh_instance_count);

    void createMeshInstance(const MeshInstance& mesh_instance, GameObjectUUID uuid);
    MeshInstance* getMeshInstancePtr(GameObjectUUID uuid);
    void clear();

public:
    uint32_t current_instance_count;
    std::shared_ptr<Renderer::Buffer> mesh_instance_buffer;
    std::map<GameObjectUUID, uint32_t> game_object_uuid_to_mesh_instance_index_map;
    std::map<uint32_t, GameObjectUUID> mesh_instance_index_to_game_object_uuid_map;
    MeshInstance* mesh_instance_buffer_ptr;
};

}  // namespace wen