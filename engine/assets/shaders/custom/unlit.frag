#version 450

// 内置自定义材质片元着色器(MVP 演示):纯色 + 强度,可选乘顶点色。
layout(std430, binding = 1) readonly buffer CustomParams {
    vec4 base_color;  // rgb + intensity(w)
    vec4 mode;        // x: 0 = unlit, 1 = 乘顶点色
} params;

layout(location = 0) in vec3 world_pos;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vcolor;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 color = params.base_color.rgb * params.base_color.w;
    if (params.mode.x > 0.5) {
        color *= vcolor;
    }
    out_color = vec4(color, 1.0);
}