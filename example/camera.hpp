#pragma once

#include <wen.hpp>
#include <glm/glm.hpp>

class Camera final {
public:
    struct CameraData {
        alignas(16) glm::vec3 position{0.0f, 0.0f, 0.0f};
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 project;
    } data;

    glm::vec3 direction;
    std::shared_ptr<wen::Renderer::UniformBuffer> uniform_buffer;

    bool is_cursor_locked = false;

public:
    Camera();
    void setInitialState(const glm::vec3& position, const glm::vec3& direction);
    void setViewportSize(float width, float height);
    void update(float ts);
    void upload();
    void reset();

private:
    glm::vec2 viewport_size_{1.0f, 1.0f};
    glm::vec3 initial_position_{0.0f, 0.0f, 3.0f};
    glm::vec3 initial_direction_{0.0f, 0.0f, -1.0f};
};