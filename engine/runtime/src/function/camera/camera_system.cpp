#include "function/camera/camera_system.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include "engine/global_context.hpp"

namespace wen {

CameraSystem::CameraSystem() {
    current_camera_id_ = 0;
    current_primary_viewport_ = 0;

    viewport_data_ = CameraData{
        .view = glm::mat4(1),
        .project = glm::ortho<float>(0, 1, 0, 1, 0, 1)
    };
    clip_data_ = viewport_data_;

    auto make_camera_buffer = []() {
        return std::make_shared<Renderer::InFlightBuffer>(
            sizeof(CameraData),
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );
    };
    viewport_camera_ = make_camera_buffer();
    clip_camera_ = make_camera_buffer();

    fixed_clip_ = false;
    editor_camera_active_ = false;
}

CameraSystem::~CameraSystem() {
    viewport_camera_.reset();
    clip_camera_.reset();
}

CameraID CameraSystem::addCamera(bool is_editor_camera) {
    current_camera_id_++;
    cameras_.insert({
        current_camera_id_,
        CameraData{
            .view = glm::mat4(1),
            .project = glm::ortho<float>(0, 1, 0, 1, 0, 1)
        }
    });
    if (current_primary_viewport_ == 0 && !is_editor_camera) {
        reportCameraAsPrimaryViewport(current_camera_id_);
    }
    return current_camera_id_;
}

void CameraSystem::removeCamera(CameraID id) {
    cameras_.erase(id);
    if (current_primary_viewport_ == id) {
        // No main camera anymore; renderers check hasActiveViewportCamera()
        // and skip scene drawing instead of reusing stale matrices.
        current_primary_viewport_ = 0;
    }
}

void CameraSystem::reportCameraViewMatrix(CameraID id, const glm::mat4& view, bool is_editor_camera) {
    auto& camera_data = cameras_.at(id);
    camera_data.view = view;
    if (editor_camera_active_ && (!is_editor_camera)) {
        return;
    }
    if (id == current_primary_viewport_ || is_editor_camera) {
        viewport_data_.view = view;
        if (!fixed_clip_) {
            clip_data_.view = view;
        }
    }
}

void CameraSystem::reportCameraProjectMatrix(CameraID id, const glm::mat4& project, float near, float far, bool is_editor_camera) {
    auto& camera_data = cameras_.at(id);
    camera_data.project = project;
    camera_data.near = near;
    camera_data.far = far;
    if (editor_camera_active_ && (!is_editor_camera)) {
        return;
    }
    if (id == current_primary_viewport_ || is_editor_camera) {
        viewport_data_.project = project;
        viewport_data_.near = near;
        viewport_data_.far = far;
        if (!fixed_clip_) {
            clip_data_.project = project;
            clip_data_.near = near;
            clip_data_.far = far;
        }
    }
}

void CameraSystem::reportCameraAsPrimaryViewport(CameraID id) {
    current_primary_viewport_ = id;
    if (editor_camera_active_) {
        return;
    }
    viewport_data_ = cameras_.at(id);
    if (!fixed_clip_) {
        clip_data_ = viewport_data_;
    }
}

void CameraSystem::turnOnFixedClip() {
    fixed_clip_ = true;
}

void CameraSystem::turnOffFixedClip() {
    fixed_clip_ = false;
    clip_data_ = viewport_data_;
}

void CameraSystem::activeEditorCamera(CameraID id) {
    editor_camera_active_ = true;
    viewport_data_ = cameras_.at(id);
    if (!fixed_clip_) {
        clip_data_ = viewport_data_;
    }
}

void CameraSystem::deactiveEditorCamera() {
    editor_camera_active_ = false;
    if (hasMainCamera()) {
        reportCameraAsPrimaryViewport(current_primary_viewport_);
    }
}

void CameraSystem::uploadFrameData(uint32_t in_flight_index) {
    memcpy(viewport_camera_->map(in_flight_index), &viewport_data_, sizeof(CameraData));
    memcpy(clip_camera_->map(in_flight_index), &clip_data_, sizeof(CameraData));
}

}  // namespace wen
