#pragma once

#include "function/render/mesh/mesh_instance_pool.hpp"

namespace wen {

class SwapData {
public:
    SwapData();
    ~SwapData();

    void clear();

    auto getMeshInstancePool() { return mesh_instance_pool_.get(); }

private:
    std::unique_ptr<MeshInstancePool> mesh_instance_pool_;
};

}  // namespace wen