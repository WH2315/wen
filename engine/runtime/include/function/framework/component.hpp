#pragma once

#include "function/framework/rtti.hpp"
#include "function/framework/uuid_manager.hpp"

namespace wen {

class Component : public RTTI {
    REFLECT_CLASS("Component")
    friend class GameObject;

public:
    std::string getClassName() const override { return "Component"; }
    static std::string GetClassName() { return "Component"; }

    virtual ~Component() = default;

    virtual void onCreate() {}
    virtual void onAwake() {}
    virtual void onStart() {}
    virtual void onFixedTick() {}
    virtual void onTick(float dt) {}
    virtual void onPostTick(float dt) {}
    virtual void onDestroy() {}

public:
    uint32_t addMemberUpdateCallback(std::function<void(Component*)> callback) {
        member_update_callbacks_.insert({current_member_update_callback_id_, std::move(callback)});
        return current_member_update_callback_id_++;
    }

    void removeMemberUpdateCallback(uint32_t id) {
        member_update_callbacks_.erase(id);
    }

    void triggerMemberUpdateCallbacks() {
        for (const auto& [id, callback] : member_update_callbacks_) {
            callback(this);
        }
    }

    auto getComponentTypeUUID() const { return uuid_; }

protected:
    ALLOW_PRIVATE_REFLECT()

    REFLECT_MEMBER()
    class GameObject* master_;
    ComponentTypeUUID uuid_;
    uint32_t current_member_update_callback_id_ = 0;
    std::map<uint32_t, std::function<void(Component*)>> member_update_callbacks_;
};

}  // namespace wen