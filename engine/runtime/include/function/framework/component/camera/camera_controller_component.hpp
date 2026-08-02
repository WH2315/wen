#pragma once

#include "function/framework/game_object.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "engine/global_context.hpp"
#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

namespace wen {

class CameraControllerComponent : public Component {
    REFLECT_CLASS("CameraControllerComponent")

public:
    std::string getClassName() const override { return "CameraControllerComponent"; }
    static std::string GetClassName() { return "CameraControllerComponent"; }

    void onStart() override {
        transform_component = game_object_->queryComponent<TransformComponent>();
        if (transform_component == nullptr) {
            WEN_CORE_ERROR("CameraControllerComponent requires a TransformComponent.");
        }
    }

    void onTick(float dt) override {
        if (transform_component == nullptr) {
            return;
        }
        auto& location = transform_component->location;
        auto& rotation = transform_component->rotation;
        bool changed = false;

        auto mouse_delta = static_cast<double>(sensitivity * dt) * global_context->input_system->getMouseDelta();
        if (global_context->input_system->isMousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
            rotation.y -= mouse_delta.x;
            rotation.x = std::clamp<float>(rotation.x + mouse_delta.y, -89.0f, 89.0f);
            changed = true;
        }

        auto delta = dt * speed;
        auto tangent = delta * glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(rotation.y));
        auto bitangent = glm::rotateY(tangent, glm::radians(90.0f));

        if (global_context->input_system->isKeyPressed(GLFW_KEY_W)) {
            location += tangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_S)) {
            location -= tangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_A)) {
            location += bitangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_D)) {
            location -= bitangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_Q)) {
            location.y -= delta;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_E)) {
            location.y += delta;
            changed = true;
        }
        if (changed) {
            transform_component->triggerMemberUpdateCallbacks();
        }
    }

    TransformComponent* transform_component = nullptr;

    REFLECT_MEMBER()
    float speed = 10;

    REFLECT_MEMBER()
    float sensitivity = 10;
};

}  // namespace wen
