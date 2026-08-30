#version 450

// 内置自定义材质片元着色器:通用参数闭环演示。
// 读取参数 SSBO 的 slot2+ (通用参数):
//   slot2 = param0(颜色, .mat 的 params 首个颜色项)
//   slot3.x = param1.x(标量, .mat 的 params 首个标量项)
// 用 param0 颜色与 base_color(slot0) 按 param1 混合,验证 Inspector 改参数能驱动渲染。
layout(std430, binding = 1) readonly buffer CustomParams {
    vec4 base_color;  // slot0: rgb + intensity(w)
    vec4 mode;        // slot1: x = 0/1 乘顶色
    vec4 param0;      // slot2: 颜色 A
    vec4 param1;      // slot3: 混合系数(mix factor)
} params;

layout(location = 0) in vec3 world_pos;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vcolor;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 base = params.base_color.rgb * params.base_color.w;  // Inspector 可改 base_color
    vec3 tint = params.param0.rgb;                            // Inspector 可改 color_a (slot2)
    float mix_factor = clamp(params.param1.x, 0.0, 1.0);      // Inspector 可改 mix_factor
    vec3 color = mix(base, tint, mix_factor);
    if (params.mode.x > 0.5) {
        color *= vcolor;
    }
    out_color = vec4(color, 1.0);
}