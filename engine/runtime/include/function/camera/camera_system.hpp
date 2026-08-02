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
    void reportCameraProjectMatrix(CameraID id, const glm::mat4& project, float near, float far, bool is_editor_camera = false);
    void reportCameraAsPrimaryViewport(CameraID id);
    void turnOnFixedClip();
    void turnOffFixedClip();

    void setMainCamera(CameraID id) { reportCameraAsPrimaryViewport(id); }
    CameraID getMainCamera() const { return current_primary_viewport_; }
    bool hasMainCamera() const { return current_primary_viewport_ != 0; }
    bool hasActiveViewportCamera() const { return editor_camera_active_ || hasMainCamera(); }

    void activeEditorCamera(CameraID id);
    void deactiveEditorCamera();

    void uploadFrameData(uint32_t in_flight_index);

    auto queryCameraData(CameraID id) const { return &cameras_.at(id); }

    auto getViewportCamera() { return viewport_camera_; }
    auto getClipCamera() { return clip_camera_; }
    bool isFixedClip() const { return fixed_clip_; }

private:
    std::map<CameraID, CameraData> cameras_;
    CameraID current_camera_id_;
    CameraID current_primary_viewport_;

    CameraData viewport_data_;
    CameraData clip_data_;
    std::shared_ptr<Renderer::InFlightBuffer> viewport_camera_;
    std::shared_ptr<Renderer::InFlightBuffer> clip_camera_;

    bool fixed_clip_;
    bool editor_camera_active_;
};

}  // namespace wen
