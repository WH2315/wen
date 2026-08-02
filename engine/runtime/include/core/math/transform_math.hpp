#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace wen::math {

// THE authoritative euler/model conventions of the engine. The GPU mirror is
// compute_model() in engine/assets/shaders/utils.glsl - keep them in sync:
//   rotation: euler angles in DEGREES, applied X, then Y, then Z (Rz*Ry*Rx)
//   model:    translate * rotation * scale (local-space scale, standard TRS)

inline glm::mat3 composeRotation(const glm::vec3& euler_degrees) {
    return glm::mat3(
        glm::eulerAngleZ(glm::radians(euler_degrees.z)) *
        glm::eulerAngleY(glm::radians(euler_degrees.y)) *
        glm::eulerAngleX(glm::radians(euler_degrees.x)));
}

inline glm::mat3 composeModel(const glm::vec3& euler_degrees, const glm::vec3& scale) {
    return composeRotation(euler_degrees) *
           glm::mat3(scale.x, 0, 0, 0, scale.y, 0, 0, 0, scale.z);
}

// Basis vectors of a rotated frame (identity rotation faces +Z, +Y up).
inline glm::vec3 forwardOf(const glm::vec3& euler_degrees) {
    return composeRotation(euler_degrees) * glm::vec3(0.0f, 0.0f, 1.0f);
}

inline glm::vec3 upOf(const glm::vec3& euler_degrees) {
    return composeRotation(euler_degrees) * glm::vec3(0.0f, 1.0f, 0.0f);
}

}  // namespace wen::math
