#include "function/render/swap_data.hpp"
#include "engine/global_context.hpp"

namespace wen {

SwapData::SwapData() {
    mesh_instance_pool_ = std::make_unique<MeshInstancePool>(global_context->render_system->getMaxMeshInstanceCount());
}

void SwapData::clear() {
    mesh_instance_pool_->clear();
}

SwapData::~SwapData() {
    mesh_instance_pool_.reset();
}

}  // namespace wen