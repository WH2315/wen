#pragma once

#include "function/framework/uuid_manager.hpp"
#include <memory>

namespace wen {

class ColliderComponent;
class RigidbodyComponent;
class TransformComponent;

// 物理系统(基于 Jolt Physics)。
// - 所有 Jolt 类型隐藏在 PIMPL 里, 本头文件不暴露任何 Jolt 头。
// - 步进集中在固定线程(fixedTick), 动态体结果经 outbox 延迟到主线程
//   (applyPendingResults) 写回 Transform 并触发成员回调, 避免跨线程触碰渲染数据。
// - beginPlay/endPlay 由 Engine::startTimer/stopTimer 驱动, Edit 模式 active=false。
class PhysicsSystem final {
    friend class Singleton<PhysicsSystem>;
    PhysicsSystem();
    ~PhysicsSystem();

public:
    // 进/出 Play 模式。active 门控 body 注册与步进。
    void beginPlay();
    void endPlay();

    // 按 GameObject 注册/反注册刚体(由 RigidbodyComponent::onStart/onDestroy 调用, 幂等)。
    void registerRigidbody(GameObjectUUID uuid,
                           ColliderComponent* collider,
                           RigidbodyComponent* rigidbody,
                           TransformComponent* transform);
    void unregisterRigidbody(GameObjectUUID uuid);

    // 组件成员变更回调入口(Inspector/撤销/反序列化后触发)。
    void notifyChanged(GameObjectUUID uuid);

    // 固定线程: 同步 kinematic -> Jolt Update -> 动态体读回 outbox。
    void fixedTick();

    // 主线程: 把 outbox 里动态体结果写回 Transform 并触发成员回调。
    void applyPendingResults();

private:
    // 以下辅助方法仅在调用方已持有 impl_->mutex_ 的前提下使用。
    void createBodyLocked(GameObjectUUID uuid, const ColliderComponent& collider,
                          const RigidbodyComponent& rigidbody, const TransformComponent& transform);
    void rebuildBodyLocked(GameObjectUUID uuid);
    void removeBodyLocked(GameObjectUUID uuid);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace wen
