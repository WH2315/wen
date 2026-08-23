#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <cmath>
#include <algorithm>

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

// Inverse of composeRotation: extract euler (DEGREES, Rz*Ry*Rx, matching the
// authoritative convention above) from a pure rotation matrix.
inline glm::vec3 decomposeRotation(const glm::mat3& R) {
    // R = Rz(z)Ry(y)Rx(x), glm 为列主序,这里 R[col][row]。
    // 分量关系: R20=-sin(y)， R21/R22 = cos(y)·sin(x)/cos(y)·cos(x)，
    //          R10/R00 = sin(z)·cos(y)/cos(z)·cos(y)。
    float sin_y = -R[0][2];
    sin_y = std::max(-1.0f, std::min(1.0f, sin_y));
    float y = std::asin(sin_y);
    float x, z;
    if (std::fabs(std::cos(y)) > 1e-6f) {
        x = std::atan2(R[1][2], R[2][2]);
        z = std::atan2(R[0][1], R[0][0]);
    } else {
        // 万向锁:令 z=0,由剩余分量反解 x。
        z = 0.0f;
        x = std::atan2(R[1][0], R[2][0]);
    }
    return glm::degrees(glm::vec3(x, y, z));
}

// Basis vectors of a rotated frame (identity rotation faces +Z, +Y up).
inline glm::vec3 forwardOf(const glm::vec3& euler_degrees) {
    return composeRotation(euler_degrees) * glm::vec3(0.0f, 0.0f, 1.0f);
}

inline glm::vec3 upOf(const glm::vec3& euler_degrees) {
    return composeRotation(euler_degrees) * glm::vec3(0.0f, 1.0f, 0.0f);
}

}  // namespace wen::math
