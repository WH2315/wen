#include "function/light/light_system.hpp"
#include "engine/global_context.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "function/framework/component/light/directional_light_component.hpp"
#include "function/framework/component/light/point_light_component.hpp"
#include "function/framework/component/light/spot_light_component.hpp"
#include <glm/gtc/constants.hpp>

namespace wen {

namespace {

// GPU 缓冲布局:首 4 字节是光源数,随后 kMaxLights 个 LightData。
struct LightBuffer {
    uint32_t light_count;
    LightData lights[LightSystem::kMaxLights];
};

}  // namespace

LightSystem::LightSystem() {
    lights_buffer_ = std::make_shared<Renderer::InFlightBuffer>(
        sizeof(LightBuffer),
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
}

LightSystem::~LightSystem() {
    lights_buffer_.reset();
}

void LightSystem::uploadFrameData(uint32_t in_flight_index) {
    auto* buffer = static_cast<LightBuffer*>(lights_buffer_->map(in_flight_index));
    buffer->light_count = 0;

    auto* scene = global_context->scene_manager->getActiveScene();
    if (scene == nullptr) {
        return;
    }
    for (auto* go : scene->getGameObjects()) {
        if (buffer->light_count >= kMaxLights) {
            break;
        }
        glm::vec3 position(0.0f);
        if (auto* transform = go->queryComponent<TransformComponent>()) {
            position = transform->location;
        }
        if (auto* light = go->queryComponent<DirectionalLightComponent>()) {
            buffer->lights[buffer->light_count++] = {
                glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                glm::vec4(light->color, light->intensity),
                glm::vec4(0.0f),
                glm::vec4(light->direction, 0.0f)};
        }
        if (auto* light = go->queryComponent<PointLightComponent>()) {
            buffer->lights[buffer->light_count++] = {
                glm::vec4(position, 1.0f),
                glm::vec4(light->color, light->intensity),
                glm::vec4(light->range, 0.0f, 0.0f, 0.0f),
                glm::vec4(0.0f)};
        }
        if (auto* light = go->queryComponent<SpotLightComponent>()) {
            float inner_cos = std::cos(glm::radians(light->inner_angle_deg));
            float outer_cos = std::cos(glm::radians(light->outer_angle_deg));
            buffer->lights[buffer->light_count++] = {
                glm::vec4(position, 2.0f),
                glm::vec4(light->color, light->intensity),
                glm::vec4(light->range, inner_cos, outer_cos, 0.0f),
                glm::vec4(light->direction, 0.0f)};
        }
    }
}

}  // namespace wen
