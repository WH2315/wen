#include "camera.hpp"
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace {
glm::vec2 s_last_cursor{0.0f, 0.0f};
bool s_first_cursor = true;
bool s_space_down = false;
}  // namespace

Camera::Camera() {
    uniform_buffer =
        std::make_shared<wen::Renderer::UniformBuffer>(sizeof(CameraData));
    setViewportSize(wen::Renderer::renderer_config.swapchain_image_width,
                    wen::Renderer::renderer_config.swapchain_image_height);
    reset();
}

void Camera::setInitialState(const glm::vec3& position,
                             const glm::vec3& direction) {
    initial_position_ = position;
    initial_direction_ = glm::length(direction) > 0.0001f
                             ? glm::normalize(direction)
                             : glm::vec3(0.0f, 0.0f, -1.0f);
    reset();
}

void Camera::setViewportSize(float width, float height) {
    viewport_size_.x = std::max(1.0f, width);
    viewport_size_.y = std::max(1.0f, height);
}

void Camera::update(float ts) {
    GLFWwindow* window =
        wen::global_context->window_system->getRuntimeWindow()->getWindow();

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 now = {x, y};
    if (s_first_cursor) {
        s_first_cursor = false;
        s_last_cursor = now;
        return;
    }

    int space_state = glfwGetKey(window, GLFW_KEY_SPACE);
    if (!s_space_down && space_state == GLFW_PRESS) {
        s_space_down = true;
    } else if (s_space_down && space_state == GLFW_RELEASE) {
        s_space_down = false;
        is_cursor_locked = !is_cursor_locked;
        glfwSetInputMode(
            window, GLFW_CURSOR,
            is_cursor_locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    if (!is_cursor_locked) {
        s_last_cursor = now;
        return;
    }

    constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
    glm::vec3 right_direction = glm::cross(direction, up_direction);
    float speed = 3.0f;
    if (glfwGetKey(window, GLFW_KEY_W)) {
        data.position += direction * speed * ts;
    } else if (glfwGetKey(window, GLFW_KEY_S)) {
        data.position -= direction * speed * ts;
    }
    if (glfwGetKey(window, GLFW_KEY_A)) {
        data.position -= right_direction * speed * ts;
    } else if (glfwGetKey(window, GLFW_KEY_D)) {
        data.position += right_direction * speed * ts;
    }
    if (glfwGetKey(window, GLFW_KEY_Q)) {
        data.position += up_direction * speed * ts;
    } else if (glfwGetKey(window, GLFW_KEY_E)) {
        data.position -= up_direction * speed * ts;
    }

    glm::vec2 delta = (now - s_last_cursor) * 0.002f;
    s_last_cursor = now;
    if (delta.x != 0.0f || delta.y != 0.0f) {
        float pitch = delta.y * 0.3f;
        float yaw = delta.x * 0.3f;
        glm::quat q =
            glm::normalize(glm::cross(glm::angleAxis(-pitch, right_direction),
                                      glm::angleAxis(-yaw, up_direction)));
        direction = glm::rotate(q, direction);
    }

    upload();
}

void Camera::upload() {
    data.view = glm::lookAt(data.position, data.position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
    auto w = std::max(1.0f, viewport_size_.x);
    auto h = std::max(1.0f, viewport_size_.y);
    data.project = glm::perspective(glm::radians(60.0f), w / h, 0.1f, 100.0f);
    memcpy(uniform_buffer->getData(), &data, sizeof(CameraData));
}

void Camera::reset() {
    data.position = initial_position_;
    direction = initial_direction_;
    is_cursor_locked = false;

    GLFWwindow* window = wen::global_context->window_system->getRuntimeWindow()->getWindow();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    s_first_cursor = true;
    s_last_cursor = {0.0f, 0.0f};
    upload();
}