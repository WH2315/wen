#include "function/camera/camera_system.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include "engine/global_context.hpp"

namespace wen {

CameraSystem::CameraSystem() {
    current_camera_id_ = 0;
    current_primary_viewport_ = 0;
    fixed_clip_ = false;

    auto interface = global_context->render_system->getInterface();
    viewport_camera_ = interface->createUniformBuffer(sizeof(CameraData));
    clip_camera_ = interface->createUniformBuffer(sizeof(CameraData));
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
        current_primary_viewport_ = 0;
    }
}

void CameraSystem::reportCameraViewMatrix(CameraID id, const glm::mat4& view, bool is_editor_camera) {
    cameras_.at(id).view = view;
    if (editor_camera_active_ && !is_editor_camera) {
        return;
    }
    if (id == current_primary_viewport_ || is_editor_camera) {
        static_cast<CameraData*>(viewport_camera_->getData())->view = view;
        if (!fixed_clip_) {
            memcpy(clip_camera_->getData(), &cameras_.at(id).view, sizeof(glm::mat4));
        }
    }
}

void CameraSystem::reportCameraProjectMatrix(CameraID id, const glm::mat4& project, bool is_editor_camera) {
    float near = project[3][2] / (project[2][2] - 1);
    float far = project[3][2] / (project[2][2] + 1);
    reportCameraProjectMatrix(id, project, near, far, is_editor_camera);
}

void CameraSystem::reportCameraProjectMatrix(CameraID id, const glm::mat4& project, float near, float far, bool is_editor_camera) {
    auto& camera_data = cameras_.at(id);
    camera_data.project = project;
    camera_data.near = near;
    camera_data.far = far;
    if (editor_camera_active_ && !is_editor_camera) {
        return;
    }
    if (id == current_primary_viewport_ || is_editor_camera) {
        static_cast<CameraData*>(viewport_camera_->getData())->project = project;
        static_cast<CameraData*>(viewport_camera_->getData())->near = near;
        static_cast<CameraData*>(viewport_camera_->getData())->far = far;
        if (!fixed_clip_) {
            static_cast<CameraData*>(clip_camera_->getData())->project = project;
            static_cast<CameraData*>(clip_camera_->getData())->near = near;
            static_cast<CameraData*>(clip_camera_->getData())->far = far;
        }
    }
}

void CameraSystem::reportCameraAsPrimaryViewport(CameraID id) {
    current_primary_viewport_ = id;
    if (editor_camera_active_) {
        return;
    }
    memcpy(viewport_camera_->getData(), &cameras_.at(id), sizeof(CameraData));
    if (!fixed_clip_) {
        memcpy(clip_camera_->getData(), &cameras_.at(id), sizeof(CameraData));
    }
}

void CameraSystem::turnOnFixedClip() {
    fixed_clip_ = true;
}

void CameraSystem::turnOffFixedClip() {
    fixed_clip_ = false;
    memcpy(clip_camera_->getData(), viewport_camera_->getData(), sizeof(CameraData));
}

void CameraSystem::activeEditorCamera(CameraID id) {
    editor_camera_active_ = true;
    memcpy(viewport_camera_->getData(), &cameras_.at(id), sizeof(CameraData));
    if (!fixed_clip_) {
        memcpy(clip_camera_->getData(), &cameras_.at(id), sizeof(CameraData));
    }
}

void CameraSystem::deactiveEditorCamera() {
    editor_camera_active_ = false;
    reportCameraAsPrimaryViewport(current_primary_viewport_);
}

}  // namespace wen