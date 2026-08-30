#pragma once

#include "core/base/singleton.hpp"
#include "function/asset/mesh_pool.hpp"
#include "function/asset/texture_pool.hpp"

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

    // 加载纹理(缓存),返回纹理池索引;失败回退 0(默认白色)。
    TextureID loadTexture(const std::string& filename) { return texture_pool_->uploadTexture(filename); }
    std::string getTextureFilename(TextureID id) const { return texture_pool_->getTextureFilename(id); }

    // 法线贴图纹理池(与 albedo 分开,便于不同兜底语义)。
    TextureID loadNormalTexture(const std::string& filename) { return normal_texture_pool_->uploadTexture(filename); }
    auto getNormalTexturePool() const { return normal_texture_pool_.get(); }

    // metallic-roughness / AO 纹理池。
    TextureID loadMrTexture(const std::string& filename) { return mr_texture_pool_->uploadTexture(filename); }
    auto getMrTexturePool() const { return mr_texture_pool_.get(); }
    TextureID loadAoTexture(const std::string& filename) { return ao_texture_pool_->uploadTexture(filename); }
    auto getAoTexturePool() const { return ao_texture_pool_.get(); }

    auto getMaxPrimitiveCount() const { return 4096; }
    auto getMaxMeshCount() const { return 1024; }
    auto getMeshPool() const { return mesh_pool_.get(); }
    auto getTexturePool() const { return texture_pool_.get(); }

private:
    std::string path_;
    std::unique_ptr<MeshPool> mesh_pool_;
    std::unique_ptr<TexturePool> texture_pool_;
    std::unique_ptr<TexturePool> normal_texture_pool_;
    std::unique_ptr<TexturePool> mr_texture_pool_;
    std::unique_ptr<TexturePool> ao_texture_pool_;
    std::map<std::string, MeshID> loaded_meshes_;
};

}  // namespace wen