#version 450

// 内置自定义材质片元着色器:线框实色。真正以线框显示依赖 .mat 的
// "wireframe": true(管线以 polygonMode=Line 绘制);本片元只输出颜色。
layout(std430, binding = 1) readonly buffer CustomParams {
    vec4 base_color;  // rgb + intensity(w)
    vec4 mode;        // x: 0 = 默认, 1 = 乘顶点色
} params;

layout(location = 3) in vec3 vcolor;

layout(location = 0) out vec4 out_color;

void main() {
    vec3 color = params.base_color.rgb * params.base_color.w;
    if (params.mode.x > 0.5) {
        color *= vcolor;
    }
    out_color = vec4(color, 1.0);
}