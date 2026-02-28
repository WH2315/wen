#include "function/render/interface/resource/push_constants.hpp"
#include "function/render/interface/basic/utils.hpp"
#include "core/base/macro.hpp"

namespace wen::Renderer {

PushConstants::PushConstants(ShaderStages stages, const std::list<std::pair<std::string, ConstantType>>& infos) {
    total_size = 0;
    for (const auto info : infos) {
        uint32_t size = convert<uint32_t>(info.second);
        uint32_t align = 4;
        if (size > 4 && size <= 8) {
            align = 8;
        } else if (size > 8) {
            align = 16;
        }
        if (total_size % align != 0) {
            total_size += align - total_size % align;
        }
        offsets_.insert(std::make_pair(info.first, total_size));
        sizes_.insert(std::make_pair(info.first, size));
        total_size += size;
    }
    range.setStageFlags(convert<vk::ShaderStageFlags>(stages))
        .setOffset(0)
        .setSize(total_size);
    constants.resize(total_size);
}

PushConstants::~PushConstants() {
    constants.clear();
    sizes_.clear();
    offsets_.clear();
}

void PushConstants::pushConstant(const std::string& name, const void* data) {
    if (offsets_.find(name) == offsets_.end()) {
        WEN_CORE_ERROR("pushConstant \"{}\" not found", name)
    }
    memcpy(constants.data() + offsets_.at(name), data, sizes_.at(name));
}

}  // namespace wen::Renderer