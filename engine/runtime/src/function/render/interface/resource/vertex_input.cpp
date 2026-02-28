#include "function/render/interface/resource/vertex_input.hpp"
#include "function/render/interface/basic/utils.hpp"

namespace wen::Renderer {

VertexInput::VertexInput(const std::vector<VertexInputInfo>& infos) {
    for (auto info : infos) {
        uint32_t offset = 0, size;
        for (auto format : info.formats) {
            attribute_descriptions_.push_back({
                location_,
                info.binding,
                convert<vk::Format>(format),
                offset
            });
            size = convert<uint32_t>(format);
            size <= 16 ? location_ += 1 : location_ += 2;
            offset += size;
        }
        binding_descriptions_.push_back({
            info.binding,
            offset,
            convert<vk::VertexInputRate>(info.input_rate)
        });
    }    
}

VertexInput::~VertexInput() {
    attribute_descriptions_.clear();
    binding_descriptions_.clear();
}

}  // namespace wen::Renderer