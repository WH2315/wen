#pragma once

#include "function/framework/component/camera/camera_component.hpp"
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

    ~CameraControllerComponent() override {
        if (transform_component == nullptr) {
            delete location;
            delete yaw;
            delete pitch;
        }
    }

    void onStart() override {
        for (auto camera_component_class_name : {"CameraComponent", "OrthographicCameraComponent", "PerspectiveCameraComponent"}) {
            camera_component = static_cast<CameraComponent*>(master_->queryComponent(camera_component_class_name));
            if (camera_component != nullptr) {
                break;
            }
        }
        if (camera_component == nullptr) {
            WEN_CORE_ERROR("CameraControllerComponent requires a CameraComponent.");
            return;
        }

        transform_component = master_->queryComponent<TransformComponent>();
        if (transform_component == nullptr) {
            location = new glm::vec3(0, 0, 0);
            yaw = new float(0);
            pitch = new float(0);
        } else {
            location = &transform_component->location;
            yaw = &transform_component->rotation.y;
            pitch = &transform_component->rotation.x;
            transform_component->addMemberUpdateCallback([this](Component*) {
                updateViewMatrix();
            });
        }

        updateViewMatrix();
    }

    void onTick(float dt) override {
        auto mouse_delta = static_cast<double>(sensitivity * dt) * global_context->input_system->getMouseDelta();
        bool changed = false;
        if (global_context->input_system->isMousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
            *yaw -= mouse_delta.x;
            *pitch = std::clamp<float>(*pitch + mouse_delta.y, -89.0f, 89.0f);
            changed = true;
        }

        auto delta = dt * speed;
        auto tangent = delta * glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(*yaw));
        auto bitangent = glm::rotateY(tangent, glm::radians(90.0f));

        if (global_context->input_system->isKeyPressed(GLFW_KEY_W)) {
            *location += tangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_S)) {
            *location -= tangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_A)) {
            *location += bitangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_D)) {
            *location -= bitangent;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_SPACE)) {
            location->y += delta;
            changed = true;
        }
        if (global_context->input_system->isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            location->y -= delta;
            changed = true;
        }
        if (changed) {
            updateViewMatrix();
        }
    }

    CameraComponent* camera_component;
    TransformComponent* transform_component;
    glm::vec3* location;
    float* yaw;
    float* pitch;

    REFLECT_MEMBER()
    float speed = 10;

    REFLECT_MEMBER()
    float sensitivity = 10;

private:
    void updateViewMatrix() {
        auto view_direction = glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(*pitch)), glm::radians(*yaw));
        global_context->camera_system->reportCameraViewMatrix(camera_component->camera_id, glm::lookAt(*location, *location + view_direction, {0, 1, 0}));
    }
};

}  // namespace wen