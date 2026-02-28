#include "function/render/interface/basic/enums.hpp"
#include "function/render/interface/basic/utils.hpp"
#include <glm/glm.hpp>

namespace wen::Renderer {

template <>
vk::VertexInputRate convert<vk::VertexInputRate>(InputRate input_rate) {
    switch (input_rate) {
        case InputRate::eVertex: return vk::VertexInputRate::eVertex;
        case InputRate::eInstance: return vk::VertexInputRate::eInstance;
    }
}

template <>
vk::Format convert<vk::Format>(VertexType format) {
    switch (format) {
        case VertexType::eInt32: return vk::Format::eR32Sint;
        case VertexType::eInt32x2: return vk::Format::eR32G32Sint;
        case VertexType::eInt32x3: return vk::Format::eR32G32B32Sint;
        case VertexType::eInt32x4: return vk::Format::eR32G32B32A32Sint;
        case VertexType::eUint32: return vk::Format::eR32Uint;
        case VertexType::eUint32x2: return vk::Format::eR32G32Uint;
        case VertexType::eUint32x3: return vk::Format::eR32G32B32Uint;
        case VertexType::eUint32x4: return vk::Format::eR32G32B32A32Uint;
        case VertexType::eFloat: return vk::Format::eR32Sfloat;
        case VertexType::eFloat2: return vk::Format::eR32G32Sfloat;
        case VertexType::eFloat3: return vk::Format::eR32G32B32Sfloat;
        case VertexType::eFloat4: return vk::Format::eR32G32B32A32Sfloat;
    }
}

template <>
uint32_t convert<uint32_t>(VertexType format) {
    switch (format) {
        case VertexType::eInt32: return 4;
        case VertexType::eInt32x2: return 8;
        case VertexType::eInt32x3: return 12;
        case VertexType::eInt32x4: return 16;
        case VertexType::eUint32: return 4;
        case VertexType::eUint32x2: return 8;
        case VertexType::eUint32x3: return 12;
        case VertexType::eUint32x4: return 16;
        case VertexType::eFloat: return 4;
        case VertexType::eFloat2: return 8;
        case VertexType::eFloat3: return 12;
        case VertexType::eFloat4: return 16;
    }
}

template <>
vk::IndexType convert<vk::IndexType>(IndexType type) {
    switch (type) {
        case IndexType::eUint16: return vk::IndexType::eUint16;
        case IndexType::eUint32: return vk::IndexType::eUint32;
    }
}

template <>
uint32_t convert<uint32_t>(IndexType type) {
    switch (type) {
        case IndexType::eUint16: return 2;
        case IndexType::eUint32: return 4;
    }
}

template <>
vk::ShaderStageFlags convert<vk::ShaderStageFlags>(ShaderStages stages) {
    return vk::ShaderStageFlags(static_cast<uint32_t>(stages));
}

template <>
vk::TransformMatrixKHR convert<vk::TransformMatrixKHR>(const glm::mat4 &matrix) {
    vk::TransformMatrixKHR result;
    auto matrix_t = glm::transpose(matrix);
    memcpy(&result, &matrix_t, sizeof(vk::TransformMatrixKHR));
    return result;
}

}  // namespace wen::Renderer