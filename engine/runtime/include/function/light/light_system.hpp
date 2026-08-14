#pragma once

#include "core/base/singleton.hpp"
#include "function/render/interface/resource/buffer.hpp"
#include <glm/glm.hpp>

namespace wen {

// 光源数据(与 GLSL Light 结构对应,std430 布局,4 个 vec4)。
struct alignas(16) LightData {
    glm::vec4 position_type;    // .xyz: 位置(点/聚光), .w: 类型 0=方向 1=点 2=聚光
    glm::vec4 color_intensity;  // .xyz: 颜色, .w: 强度
    glm::vec4 range_angle;      // .x: 衰减范围, .y: 内锥角cos, .z: 外锥角cos
    glm::vec4 direction;        // 照射方向(方向光/聚光用)
};

// 光源系统:维护一个光源 storage buffer,每帧从活动场景收集光源组件上传,
// 供 mesh pass 着色使用。
class LightSystem final {
    friend class Singleton<LightSystem>;
    LightSystem();
    ~LightSystem();

public:
    void uploadFrameData(uint32_t in_flight_index);

    auto getLightBuffer() const { return lights_buffer_; }

    static constexpr uint32_t kMaxLights = 16;

private:
    std::shared_ptr<Renderer::InFlightBuffer> lights_buffer_;
};

}  // namespace wen
