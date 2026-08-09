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
    auto getRootDir() const { return path_; }

    MeshID loadMesh(const std::string& filename, const std::vector<std::string>& lods = {});

    // 反查网格的加载文件名(场景序列化用)
    std::string getMeshFilename(MeshID mesh_id) const {
        for (const auto& [filename, id] : loaded_meshes_) {
            if (id == mesh_id) {
                return filename;
            }
        }
        return {};
    }

    auto getMaxPrimitiveCount() const { return 4096; }
    auto getMaxMeshCount() const { return 1024; }
    auto getMeshPool() const { return mesh_pool_.get(); }

private:
    std::string path_;
    std::unique_ptr<MeshPool> mesh_pool_;
    std::map<std::string, MeshID> loaded_meshes_;
};

}  // namespace wen