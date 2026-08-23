#include "function/physics/physics_system.hpp"

#include "engine/global_context.hpp"
#include "function/framework/scene_manager.hpp"
#include "function/framework/component/physics/collider_component.hpp"
#include "function/framework/component/physics/rigidbody_component.hpp"
#include "function/framework/component/transform/transform_component.hpp"
#include "core/math/transform_math.hpp"
#include "core/base/macro.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace wen {

// ---------------------------------------------------------------------------
// 对象层 / 宽相位过滤器(两个 ObjectLayer: 静态×静态不碰撞, 其余可碰撞)。
// ---------------------------------------------------------------------------
namespace {

namespace Layers {
static constexpr JPH::ObjectLayer kNonMoving = 0;
static constexpr JPH::ObjectLayer kMoving = 1;
}  // namespace Layers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        return JPH::BroadPhaseLayer(inLayer == Layers::kMoving ? 1 : 0);
    }
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        return inLayer1 == Layers::kMoving || inLayer2 == JPH::BroadPhaseLayer(1);
    }
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override {
        return inLayer1 == Layers::kMoving || inLayer2 == Layers::kMoving;
    }
};

// ---------------------------------------------------------------------------
// 内部结构 + 形状/旋转换算辅助。
// ---------------------------------------------------------------------------

struct BodyEntry {
    JPH::BodyID body_id;
    int body_type;             // RigidbodyComponent::kStatic/kDynamic/kKinematic
    bool is_sensor;
    uint64_t shape_sig;
    TransformComponent* transform;  // 仅 kinematic 每步同步时读取
};

struct PendingResult {
    GameObjectUUID uuid;
    JPH::Vec3 position;
    JPH::Quat rotation;
};

// 引擎旋转约定 Rz*Ry*Rx(度) -> JPH quat(x,y,z,w)。
JPH::Quat eulerToQuat(const glm::vec3& euler_degrees) {
    glm::quat q = glm::quat(math::composeRotation(euler_degrees));
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

// glm mat3(column-major) -> 引擎约定 euler 度。反解 R = Rz·Ry·Rx。
glm::vec3 mat3ToEulerDeg(const glm::mat3& m) {
    float sy = std::clamp(-m[0][2], -1.0f, 1.0f);
    float y = std::asin(sy);
    float x = std::atan2(m[1][2], m[2][2]);
    float z = std::atan2(m[0][1], m[0][0]);
    return glm::degrees(glm::vec3(x, y, z));
}

glm::vec3 quatToEulerDeg(const JPH::Quat& r) {
    glm::quat q(r.GetW(), r.GetX(), r.GetY(), r.GetZ());
    return mat3ToEulerDeg(glm::mat3_cast(q));
}

// 由 Collider+Rigidbody 计算形状/运动签名。变化需要重建 body;
// 未进入签名的参数(friction/restitution/gravity/velocity)可原地更新。
uint64_t shapeSig(const ColliderComponent& c, const RigidbodyComponent& r) {
    auto mix = [](uint64_t& h, uint64_t v) { h = h * 1099511628211ull ^ v; };
    auto fb = [](float f) {
        uint32_t u;
        std::memcpy(&u, &f, sizeof(float));
        return static_cast<uint64_t>(u);
    };
    uint64_t h = 0;
    mix(h, static_cast<uint64_t>(c.shape_type));
    mix(h, static_cast<uint64_t>(c.is_trigger));
    mix(h, static_cast<uint64_t>(r.body_type));
    mix(h, static_cast<uint64_t>(r.use_ccd));
    mix(h, fb(r.mass));
    mix(h, fb(r.linear_damping));
    mix(h, fb(r.angular_damping));
    mix(h, fb(c.half_extents.x)); mix(h, fb(c.half_extents.y)); mix(h, fb(c.half_extents.z));
    mix(h, fb(c.radius));
    mix(h, fb(c.height));
    mix(h, fb(c.center.x)); mix(h, fb(c.center.y)); mix(h, fb(c.center.z));
    return h;
}

JPH::ShapeRefC buildShape(const ColliderComponent& c, const glm::vec3& scale) {
    JPH::ShapeRefC inner;
    switch (c.shape_type) {
        case ColliderComponent::kShapeBox:
            inner = new JPH::BoxShape(JPH::Vec3(c.half_extents.x * scale.x,
                                                c.half_extents.y * scale.y,
                                                c.half_extents.z * scale.z));
            break;
        case ColliderComponent::kShapeSphere:
            inner = new JPH::SphereShape(c.radius * scale.x);
            break;
        case ColliderComponent::kShapeCapsule: {
            float r = c.radius * scale.x;
            inner = new JPH::CapsuleShape(std::max(c.height * 0.5f * scale.y - r, 0.0f), r);
            break;
        }
        case ColliderComponent::kShapeCylinder:
            inner = new JPH::CylinderShape(c.height * 0.5f * scale.y, c.radius * scale.x);
            break;
        default:
            inner = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
            break;
    }
    if (c.center != glm::vec3(0.0f)) {
        return new JPH::RotatedTranslatedShape(JPH::Vec3(c.center.x, c.center.y, c.center.z),
                                               JPH::Quat::sIdentity(), inner.GetPtr());
    }
    return inner;
}

JPH::EMotionType motionTypeOf(int body_type) {
    if (body_type == RigidbodyComponent::kDynamic) {
        return JPH::EMotionType::Dynamic;
    }
    if (body_type == RigidbodyComponent::kKinematic) {
        return JPH::EMotionType::Kinematic;
    }
    return JPH::EMotionType::Static;
}

}  // namespace

// ---------------------------------------------------------------------------
// PhysicsSystem::Impl —— 持有全部 Jolt 对象。
// ---------------------------------------------------------------------------

struct PhysicsSystem::Impl {
    // 必须先于任何 Jolt 分配执行: TempAllocatorImpl 等成员初始化会调 JPH::Allocate,
    // 而 RegisterDefaultAllocator 必须在第一次分配前完成(否则分配器是空指针)。
    struct JoltInit {
        JoltInit() {
            JPH::RegisterDefaultAllocator();
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
        }
        ~JoltInit() {
            JPH::UnregisterTypes();
            JPH::Factory::sInstance = nullptr;
        }
    };

    Impl() {
        physics_system_.Init(65536, 0, 65536, 10240,
                             bpl_interface_, obj_vs_bp_filter_, obj_pair_filter_);
        physics_system_.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    }

    JoltInit jolt_init_;  // 第一个成员, 先于所有 Jolt 分配。
    // 顺序重要: 过滤器必须在 physics_system_ 之前构造、之后析构。
    JPH::TempAllocatorImpl temp_allocator_{16 * 1024 * 1024};
    JPH::JobSystemThreadPool job_system_{JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                         std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)};
    BPLayerInterfaceImpl bpl_interface_;
    ObjectVsBroadPhaseLayerFilterImpl obj_vs_bp_filter_;
    ObjectLayerPairFilterImpl obj_pair_filter_;
    JPH::PhysicsSystem physics_system_;

    std::mutex mutex_;
    std::unordered_map<GameObjectUUID, BodyEntry> bodies_;
    std::vector<PendingResult> pending_;
    bool active_ = false;
};

// ---------------------------------------------------------------------------
// 构造/析构。
// ---------------------------------------------------------------------------

PhysicsSystem::PhysicsSystem() : impl_(std::make_unique<Impl>()) {}
PhysicsSystem::~PhysicsSystem() = default;

void PhysicsSystem::beginPlay() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->active_ = true;
}

void PhysicsSystem::endPlay() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->active_ = false;
    auto& bi = impl_->physics_system_.GetBodyInterface();
    for (auto& [uuid, entry] : impl_->bodies_) {
        bi.RemoveBody(entry.body_id);
        bi.DestroyBody(entry.body_id);
    }
    impl_->bodies_.clear();
    impl_->pending_.clear();
}

// ---------------------------------------------------------------------------
// 注册/反注册。
// ---------------------------------------------------------------------------

void PhysicsSystem::registerRigidbody(GameObjectUUID uuid,
                                      ColliderComponent* collider,
                                      RigidbodyComponent* rigidbody,
                                      TransformComponent* transform) {
    if (collider == nullptr || rigidbody == nullptr || transform == nullptr) {
        WEN_CORE_WARN("PhysicsSystem::registerRigidbody: 缺少 Collider/Rigidbody/Transform (uuid {})", uuid)
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (!impl_->active_) {
        return;
    }
    auto it = impl_->bodies_.find(uuid);
    uint64_t sig = shapeSig(*collider, *rigidbody);
    if (it != impl_->bodies_.end() && it->second.shape_sig == sig) {
        return;  // 已存在且形状一致。
    }
    if (it != impl_->bodies_.end()) {
        auto& bi = impl_->physics_system_.GetBodyInterface();
        bi.RemoveBody(it->second.body_id);
        bi.DestroyBody(it->second.body_id);
        impl_->bodies_.erase(it);
    }
    createBodyLocked(uuid, *collider, *rigidbody, *transform);
}

void PhysicsSystem::unregisterRigidbody(GameObjectUUID uuid) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto it = impl_->bodies_.find(uuid);
    if (it == impl_->bodies_.end()) {
        return;
    }
    auto& bi = impl_->physics_system_.GetBodyInterface();
    bi.RemoveBody(it->second.body_id);
    bi.DestroyBody(it->second.body_id);
    impl_->bodies_.erase(it);
}

// ---------------------------------------------------------------------------
// 锁内辅助: 建/重建/删 body。
// ---------------------------------------------------------------------------

void PhysicsSystem::createBodyLocked(GameObjectUUID uuid,
                                     const ColliderComponent& collider,
                                     const RigidbodyComponent& rigidbody,
                                     const TransformComponent& transform) {
    JPH::ShapeRefC shape = buildShape(collider, transform.scale);
    JPH::EMotionType motion = motionTypeOf(rigidbody.body_type);
    JPH::ObjectLayer layer =
        (motion == JPH::EMotionType::Static) ? Layers::kNonMoving : Layers::kMoving;

    JPH::BodyCreationSettings settings(shape.GetPtr(),
                                       JPH::Vec3(transform.getWorldLocation().x,
                                                 transform.getWorldLocation().y,
                                                 transform.getWorldLocation().z),
                                       eulerToQuat(transform.getWorldRotation()), motion, layer);
    settings.mIsSensor = collider.is_trigger;
    settings.mFriction = rigidbody.friction;
    settings.mRestitution = rigidbody.restitution;
    settings.mLinearDamping = rigidbody.linear_damping;
    settings.mAngularDamping = rigidbody.angular_damping;
    settings.mGravityFactor = rigidbody.gravity_scale;
    settings.mMotionQuality =
        rigidbody.use_ccd ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    if (motion == JPH::EMotionType::Dynamic && rigidbody.mass > 0.0f) {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = rigidbody.mass;
    }

    auto& bi = impl_->physics_system_.GetBodyInterface();
    JPH::Body* body = bi.CreateBody(settings);
    if (body == nullptr) {
        WEN_CORE_ERROR("PhysicsSystem::createBodyLocked: CreateBody 失败 (uuid {})", uuid)
        return;
    }
    BodyEntry entry;
    entry.body_id = body->GetID();
    entry.body_type = rigidbody.body_type;
    entry.is_sensor = collider.is_trigger;
    entry.shape_sig = shapeSig(collider, rigidbody);
    entry.transform = const_cast<TransformComponent*>(&transform);
    bi.AddBody(entry.body_id, JPH::EActivation::Activate);
    bi.SetLinearAndAngularVelocity(entry.body_id,
                                   JPH::Vec3(rigidbody.linear_velocity.x,
                                             rigidbody.linear_velocity.y,
                                             rigidbody.linear_velocity.z),
                                   JPH::Vec3(rigidbody.angular_velocity.x,
                                             rigidbody.angular_velocity.y,
                                             rigidbody.angular_velocity.z));
    impl_->bodies_[uuid] = entry;
}

void PhysicsSystem::rebuildBodyLocked(GameObjectUUID uuid) {
    auto* scene = global_context->scene_manager->getActiveScene();
    auto* go = scene ? scene->getGameObject(uuid) : nullptr;
    if (go == nullptr) {
        return;
    }
    auto* collider = go->queryComponent<ColliderComponent>();
    auto* rigidbody = go->queryComponent<RigidbodyComponent>();
    auto* transform = go->queryComponent<TransformComponent>();
    if (collider == nullptr || rigidbody == nullptr || transform == nullptr) {
        removeBodyLocked(uuid);
        return;
    }
    removeBodyLocked(uuid);
    createBodyLocked(uuid, *collider, *rigidbody, *transform);
}

void PhysicsSystem::removeBodyLocked(GameObjectUUID uuid) {
    auto it = impl_->bodies_.find(uuid);
    if (it == impl_->bodies_.end()) {
        return;
    }
    auto& bi = impl_->physics_system_.GetBodyInterface();
    bi.RemoveBody(it->second.body_id);
    bi.DestroyBody(it->second.body_id);
    impl_->bodies_.erase(it);
}

// ---------------------------------------------------------------------------
// 成员变更回调。
// ---------------------------------------------------------------------------

void PhysicsSystem::notifyChanged(GameObjectUUID uuid) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (!impl_->active_) {
        return;
    }
    auto it = impl_->bodies_.find(uuid);
    if (it == impl_->bodies_.end()) {
        return;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    auto* go = scene ? scene->getGameObject(uuid) : nullptr;
    if (go == nullptr) {
        removeBodyLocked(uuid);
        return;
    }
    auto* collider = go->queryComponent<ColliderComponent>();
    auto* rigidbody = go->queryComponent<RigidbodyComponent>();
    auto* transform = go->queryComponent<TransformComponent>();
    if (collider == nullptr || rigidbody == nullptr || transform == nullptr) {
        removeBodyLocked(uuid);
        return;
    }

    uint64_t sig = shapeSig(*collider, *rigidbody);
    if (sig != it->second.shape_sig) {
        // 形状/运动类型/质量等变化 -> 重建(位置取当前 Transform)。
        removeBodyLocked(uuid);
        createBodyLocked(uuid, *collider, *rigidbody, *transform);
        return;
    }

    // 仅物理材质/重力/初速变化 -> 原地更新, 不丢动量。
    auto& bi = impl_->physics_system_.GetBodyInterface();
    bi.SetFriction(it->second.body_id, rigidbody->friction);
    bi.SetRestitution(it->second.body_id, rigidbody->restitution);
    bi.SetGravityFactor(it->second.body_id, rigidbody->gravity_scale);
    bi.SetLinearAndAngularVelocity(it->second.body_id,
                                   JPH::Vec3(rigidbody->linear_velocity.x,
                                             rigidbody->linear_velocity.y,
                                             rigidbody->linear_velocity.z),
                                   JPH::Vec3(rigidbody->angular_velocity.x,
                                             rigidbody->angular_velocity.y,
                                             rigidbody->angular_velocity.z));
}

// ---------------------------------------------------------------------------
// 固定线程步进 + 主线程结果写回。
// ---------------------------------------------------------------------------

void PhysicsSystem::fixedTick() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (!impl_->active_) {
        return;
    }
    auto& bi = impl_->physics_system_.GetBodyInterface();
    // Kinematic: 每步从 Transform 同步位置/旋转。
    for (auto& [uuid, entry] : impl_->bodies_) {
        if (entry.body_type == RigidbodyComponent::kKinematic && entry.transform != nullptr) {
            bi.SetPositionAndRotation(entry.body_id,
                                      JPH::Vec3(entry.transform->getWorldLocation().x,
                                                entry.transform->getWorldLocation().y,
                                                entry.transform->getWorldLocation().z),
                                      eulerToQuat(entry.transform->getWorldRotation()),
                                      JPH::EActivation::Activate);
        }
    }
    impl_->physics_system_.Update(1.0f / 30.0f, 1, &impl_->temp_allocator_, &impl_->job_system_);
    // 动态体读回(用 body 原点 GetPosition, 不是 GetCenterOfMassPosition)。
    impl_->pending_.clear();
    for (auto& [uuid, entry] : impl_->bodies_) {
        if (entry.body_type == RigidbodyComponent::kDynamic && !entry.is_sensor) {
            impl_->pending_.push_back({uuid, bi.GetPosition(entry.body_id),
                                       bi.GetRotation(entry.body_id)});
        }
    }
}

void PhysicsSystem::applyPendingResults() {
    std::vector<PendingResult> results;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex_);
        results.swap(impl_->pending_);
    }
    if (results.empty()) {
        return;
    }
    auto* scene = global_context->scene_manager->getActiveScene();
    for (const auto& result : results) {
        auto* go = scene ? scene->getGameObject(result.uuid) : nullptr;
        if (go == nullptr) {
            continue;  // 对象已被销毁, 丢弃结果。
        }
        auto* transform = go->queryComponent<TransformComponent>();
        if (transform == nullptr) {
            continue;
        }
        // P0 约束:动态体写回仅支持场景根对象(结果为世界坐标)。
        // 挂在父节点上的动态体不在支持范围,跳过以免把世界坐标误写进本地分量。
        if (transform->getGameObject() != nullptr &&
            transform->getGameObject()->getParent() != nullptr) {
            continue;
        }
        transform->location = glm::vec3(result.position.GetX(), result.position.GetY(),
                                        result.position.GetZ());
        transform->rotation = quatToEulerDeg(result.rotation);
        // 脏传播:动态体写回后同步其上所有后代组件(如子 Mesh)的世界变换。
        transform->propagateWorldChange();
    }
}

}  // namespace wen
