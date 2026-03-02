#pragma once

#include "core/base/singleton.hpp"
#include "function/render/interface/resource/buffer.hpp"
#include <glm/glm.hpp>

namespace wen {

using CameraID = uint32_t;

struct CameraData {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 project;
    alignas(4) float near;
    alignas(4) float far;
};

class CameraSystem final {
    friend class Singleton<CameraSystem>;
    CameraSystem();
    ~CameraSystem();

public:
    CameraID addCamera(bool is_editor_camera = false);
    void removeCamera(CameraID id);
    void reportCameraViewMatrix(CameraID id, const glm::mat4& view, bool is_editor_camera = false);
    void reportCameraProjectMatrix(CameraID id, const glm::mat4& project, bool is_editor_camera = false);
    void reportCameraProjectMatrix(CameraID id, const glm::mat4& project, float near, float far, bool is_editor_camera = false);
    void reportCameraAsPrimaryViewport(CameraID id);
    void turnOnFixedClip();
    void turnOffFixedClip();

    void activeEditorCamera(CameraID id);
    void deactiveEditorCamera();

    auto queryCameraData(CameraID id) const { return &cameras_.at(id); }
    auto queryPrimaryCameraID() const { return current_primary_viewport_; }

    auto getViewportCamera() { return viewport_camera_; }
    auto getClipCamera() { return clip_camera_; }
    bool isFixedClip() const { return fixed_clip_; }

private:
    std::map<CameraID, CameraData> cameras_;
    CameraID current_camera_id_;
    std::shared_ptr<Renderer::UniformBuffer> viewport_camera_;
    std::shared_ptr<Renderer::UniformBuffer> clip_camera_;
    CameraID current_primary_viewport_;
    bool fixed_clip_;
    bool editor_camera_active_;
};

}  // namespace wen