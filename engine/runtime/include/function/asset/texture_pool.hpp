#pragma once

#include "function/render/interface/interface.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace wen {

using TextureID = uint32_t;

// 纹理池:加载 GPU 纹理并缓存(索引 0 为默认白色纹理,无纹理时引用它)。
// 纹理与采样器按 (SpecificTexture, Sampler) 对存储,可直接喂给 DescriptorSet::bindTextures。
class TexturePool {
public:
    TexturePool(uint32_t max_texture_count);
    ~TexturePool();

    // 按 <资源根>/textures/<filename> 加载并缓存;失败/超限回退白色纹理(0)。
    TextureID uploadTexture(const std::string& filename);
    std::string getTextureFilename(TextureID id) const;

    auto getTexture(TextureID id) const {
        return id < textures_samplers_.size() ? textures_samplers_[id].first : nullptr;
    }
    auto getTextureCount() const { return textures_samplers_.size(); }
    auto getMaxTextureCount() const { return max_texture_count_; }

    // 供网格 pass 绑定全部纹理(数组)到 combined image sampler。
    const auto& texturesSamplers() const { return textures_samplers_; }

    // 补齐到 max_texture_count 的纹理/采样器对(空槽用默认白纹理)。
    std::vector<std::pair<std::shared_ptr<Renderer::SpecificTexture>, std::shared_ptr<Renderer::Sampler>>>
    texturesSamplersPadded() const {
        auto out = textures_samplers_;
        while (out.size() < max_texture_count_) {
            out.push_back(out.front());
        }
        return out;
    }

private:
    uint32_t max_texture_count_;
    std::shared_ptr<Renderer::Sampler> sampler_;
    std::vector<std::pair<std::shared_ptr<Renderer::SpecificTexture>, std::shared_ptr<Renderer::Sampler>>> textures_samplers_;
    std::map<std::string, TextureID> loaded_;
};

}  // namespace wen
