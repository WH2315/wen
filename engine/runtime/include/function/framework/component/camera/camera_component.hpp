#pragma once

#include "function/framework/component.hpp"
#include "engine/global_context.hpp"

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

    CameraID camera_id;
};

}  // namespace wen