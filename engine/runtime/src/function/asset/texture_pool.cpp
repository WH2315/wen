#include "function/asset/texture_pool.hpp"
#include "engine/global_context.hpp"
#include "core/base/macro.hpp"
#include <filesystem>

namespace wen {

namespace fs = std::filesystem;

TexturePool::TexturePool(uint32_t max_texture_count) : max_texture_count_(max_texture_count) {
    auto interface = global_context->render_system->getInterface();
    sampler_ = interface->createSampler({
        .mag_filter = vk::Filter::eLinear,
        .min_filter = vk::Filter::eLinear,
        .address_mode_u = vk::SamplerAddressMode::eRepeat,
        .address_mode_v = vk::SamplerAddressMode::eRepeat,
        .address_mode_w = vk::SamplerAddressMode::eRepeat,
    });
    // 默认白色 1x1 纹理,作为"无纹理"的兜底(索引 0)。
    uint8_t white[4] = {255, 255, 255, 255};
    textures_samplers_.push_back({interface->createTexture(white, 1, 1, 1), sampler_});
}

TexturePool::~TexturePool() {
    textures_samplers_.clear();
    sampler_.reset();
}

TextureID TexturePool::uploadTexture(const std::string& filename) {
    if (auto iter = loaded_.find(filename); iter != loaded_.end()) {
        return iter->second;
    }
    if (textures_samplers_.size() >= max_texture_count_) {
        WEN_CORE_WARN("TexturePool texture count overflow (max {}), fallback to white: {}",
                      max_texture_count_, filename)
        return 0;
    }
    auto full_path = fs::path(global_context->asset_system->getRootDir()) / "textures" / filename;
    if (!fs::exists(full_path)) {
        WEN_CORE_WARN("TexturePool texture not found: {}", full_path.string())
        return 0;
    }
    // createTexture 内部会拼 texture_dir_(<资源根>/textures),只需传文件名。
    auto interface = global_context->render_system->getInterface();
    auto texture = interface->createTexture(filename, 1);
    if (texture == nullptr || !texture->isValid()) {
        WEN_CORE_WARN("TexturePool failed to load texture: {}", filename)
        return 0;
    }
    TextureID id = static_cast<TextureID>(textures_samplers_.size());
    textures_samplers_.push_back({texture, sampler_});
    loaded_.insert({filename, id});
    return id;
}

std::string TexturePool::getTextureFilename(TextureID id) const {
    for (const auto& [filename, tid] : loaded_) {
        if (tid == id) {
            return filename;
        }
    }
    return {};
}

}  // namespace wen
