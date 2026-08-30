#version 450

// 内置自定义材质片元着色器:法线可视化(调试/教学用)。
// 把世界法线直接映射成颜色,便于检查法线方向与是否正确传递。
layout(std430, binding = 1) readonly buffer CustomParams {
    vec4 base_color;  // 未使用
    vec4 mode;
} params;

layout(location = 1) in vec3 world_normal;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 n = normalize(world_normal);
    // 法线 [-1,1] -> 颜色 [0,1]
    vec3 color = n * 0.5 + 0.5;
    out_color = vec4(color, 1.0);
}