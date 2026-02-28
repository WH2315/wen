#pragma once

#include "function/render/interface/basic/enums.hpp"

namespace wen::Renderer {

using ConstantType = VertexType;

class PushConstants {
public:
    PushConstants(ShaderStages stages, const std::list<std::pair<std::string, ConstantType>>& infos);
    ~PushConstants();

    void pushConstant(const std::string& name, const void* data);

public:
    uint32_t total_size;
    std::vector<uint8_t> constants;
    vk::PushConstantRange range;

private:
    std::map<std::string, uint32_t> offsets_;
    std::map<std::string, uint32_t> sizes_;
};

}  // namespace wen::Renderer