#pragma once

#include "function/framework/component.hpp"
#include "function/framework/game_object.hpp"
#include "core/math/transform_math.hpp"
#include <glm/gtx/matrix_decompose.hpp>

namespace wen {

class TransformComponent : public Component {
    REFLECT_CLASS("TransformComponent")

public:
    std::string getClassName() const override { return "TransformComponent"; }
    static std::string GetClassName() { return "TransformComponent"; }

    // 本地坐标:location/rotation/scale 描述相对父 Transform 的偏移。
    // 无父对象时本地==世界。旋转欧拉角单位:度,顺序 Rz*Ry*Rx。

    REFLECT_MEMBER()
    glm::vec3 location{0, 0, 0};

    REFLECT_MEMBER()
    glm::vec3 rotation{0, 0, 0};

    REFLECT_MEMBER()
    glm::vec3 scale{1, 1, 1};

    // ---- 场景图/父子层级:本地与世界变换 ----
    glm::mat4 getLocalMatrix() const {
        return glm::translate(glm::mat4(1.0f), location) *
               glm::mat4(math::composeModel(rotation, scale));
    }

    glm::mat4 getWorldMatrix() const {
        if (auto* parent_transform = getParentTransform()) {
            return parent_transform->getWorldMatrix() * getLocalMatrix();
        }
        return getLocalMatrix();
    }

    glm::vec3 getWorldLocation() const {
        return glm::vec3(getWorldMatrix()[3]);
    }

    glm::vec3 getWorldRotation() const {
        if (auto* parent_transform = getParentTransform()) {
            auto world_quat = glm::quat(math::composeRotation(parent_transform->getWorldRotation())) *
                              glm::quat(math::composeRotation(rotation));
            return math::decomposeRotation(glm::mat3_cast(world_quat));
        }
        return rotation;
    }

    glm::vec3 getWorldScale() const {
        if (auto* parent_transform = getParentTransform()) {
            return parent_transform->getWorldScale() * scale;
        }
        return scale;
    }

    // 把"目标世界变换"写回本地分量(供 Gizmo 拖拽等使用)。
    // 无父时本地==世界,直接分解;有父时先用父世界矩阵逆变换到本地再分解。
    void setFromWorldMatrix(const glm::mat4& world_matrix) {
        glm::mat4 local_matrix = world_matrix;
        if (auto* parent_transform = getParentTransform()) {
            local_matrix = glm::inverse(parent_transform->getWorldMatrix()) * world_matrix;
        }
        // 分解 TRS(T*R*S),得到本地 location/rotation/scale。
        glm::vec3 translation;
        glm::quat rotation_quat;
        glm::vec3 scale_vec;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(local_matrix, scale_vec, rotation_quat, translation, skew, perspective);
        location = translation;
        rotation = math::decomposeRotation(glm::mat3_cast(rotation_quat));
        scale = scale_vec;
    }

    // 本对象 Transform 发生变化后调用:先把世界变换推送给自身消费者(渲染实例等),
    // 再把"父节点变化"递归传播给所有后代 Transform,使它们重算世界变换并刷新。
    // 这是让"移动父节点 -> 子节点跟随"可见的关键(dirty 传播)。
    void propagateWorldChange() {
        triggerMemberUpdateCallbacks();
        if (game_object_ == nullptr) {
            return;
        }
        for (auto* child : game_object_->getChildren()) {
            if (child == nullptr) {
                continue;
            }
            if (auto* child_transform = child->queryComponent<TransformComponent>()) {
                child_transform->propagateWorldChange();
            }
        }
    }

private:
    // 父对象的 TransformComponent(无父或父无 Transform 时为 nullptr)。
    TransformComponent* getParentTransform() const {
        auto* parent = game_object_ == nullptr ? nullptr : game_object_->getParent();
        return parent == nullptr ? nullptr : parent->queryComponent<TransformComponent>();
    }
};

}  // namespace wen