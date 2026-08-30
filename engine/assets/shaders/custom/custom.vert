#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec3 in_color;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 project;
} camera;

layout(push_constant) uniform Model {
    mat4 model;
} pc;

layout(location = 0) out vec3 world_pos;
layout(location = 1) out vec3 world_normal;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec3 vcolor;

void main() {
    vec4 wp = pc.model * vec4(in_pos, 1.0);
    world_pos = wp.xyz;
    world_normal = mat3(pc.model) * in_normal;
    uv = in_uv;
    vcolor = in_color;
    gl_Position = camera.project * camera.view * wp;
}