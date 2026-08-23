#pragma once

#include "function/framework/game_object.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "engine/global_context.hpp"
#include "core/math/transform_math.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace wen {

class CameraComponent : public Component {
    REFLECT_CLASS("CameraComponent")

public:
    std::string getClassName() const override { return "CameraComponent"; }
    static std::string GetClassName() { return "CameraComponent"; }

    CameraComponent() {
        camera_id = global_context->camera_system->addCamera();
    }

    ~CameraComponent() override {
        global_context->camera_system->removeCamera(camera_id);
        camera_id = 0;
    }

    void onStart() override {
        transform_component_ = game_object_->queryComponent<TransformComponent>();
        // Scene start runs on every Edit->Game switch; register the transform
        // callback only once or they accumulate.
        if (transform_component_ != nullptr && !transform_callback_registered_) {
            transform_callback_registered_ = true;
            transform_component_->addMemberUpdateCallback([this](Component*) {
                updateViewFromTransform();
            });
        }
        updateViewFromTransform();
    }

    CameraID camera_id;

protected:
    // Rotation convention: see wen::math (matches the renderer's model matrix).
    void updateViewFromTransform() {
        glm::vec3 location(0.0f);
        glm::vec3 rotation(0.0f);
        if (transform_component_ != nullptr) {
            location = transform_component_->getWorldLocation();
            rotation = transform_component_->getWorldRotation();
        }
        global_context->camera_system->reportCameraViewMatrix(
            camera_id,
            glm::lookAt(location, location + math::forwardOf(rotation), math::upOf(rotation)));
    }

    TransformComponent* transform_component_ = nullptr;
    bool transform_callback_registered_ = false;
};

}  // namespace wen
