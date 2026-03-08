#include "function/render/render_data.hpp"
#include "engine/global_context.hpp"

namespace wen {

RenderData::RenderData() {
    mesh_instance_pool_ = std::make_unique<MeshInstancePool>(global_context->render_system->getMaxMeshInstanceCount());
}

void RenderData::clear() {
    mesh_instance_pool_->clear();
}

RenderData::~RenderData() {
    mesh_instance_pool_.reset();
}

}  // namespace wen