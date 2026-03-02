#pragma once

#include "core/base/singleton.hpp"
#include "function/asset/mesh_pool.hpp"

namespace wen {

class AssetSystem final {
    friend class Singleton<AssetSystem>;
    AssetSystem();
    ~AssetSystem();

public:
    void setRootDir(const std::string& path) { path_ = path; }

    MeshID loadMesh(const std::string& filename, const std::vector<std::string>& lods = {});

    auto getMaxPrimitiveCount() const { return 4096; }
    auto getMaxMeshCount() const { return 1024; }
    auto getMeshPool() const { return mesh_pool_.get(); }

private:
    std::string path_;
    std::unique_ptr<MeshPool> mesh_pool_;
};

}  // namespace wen