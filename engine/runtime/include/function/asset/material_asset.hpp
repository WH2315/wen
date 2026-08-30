#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace wen {

// 通用材质参数:值统一以 vec4 存储(is_color 决定显示为颜色还是标量)。
// 序列化到 SSBO 的 slot 2+;内置 base_color/mode 在 slot 0-1。
struct CustomMaterialParam {
    std::string name;
    glm::vec4 value = glm::vec4(0.0f);
    bool is_color = false;  // true: vec3 颜色; false: 标量(float 取 value.x)
};

// 自定义材质资产(.mat,JSON):
//   { "shader":"custom/unlit.frag", "base_color":[1,0,0], "intensity":1.0, "mode":0,
//     "wireframe":false, "params":{"color_a":[1,0.2,0.2],"factor":0.5} }
// 片元着色器经 loadShader(<shader>, eFragment) 加载;参数打包进一个 SSBO。
struct CustomMaterialAsset {
    std::string shader = "custom/unlit.frag";  // 相对 shaders 目录的 .frag
    glm::vec3 base_color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float mode = 0.0f;      // 0 = unlit, 1 = 乘以顶点色
    bool wireframe = false; // true = 用线框(多边形模式 Line)绘制
    std::string cull = "none"; // "none" / "front" / "back" —— 背面剔除
    std::vector<CustomMaterialParam> params;  // 通用参数(上限见 pass 的 slot 容量)
};

// 从完整文件路径解析 .mat;失败返回 false 并保留 out 默认值。
bool loadMaterialAsset(const std::string& full_path, CustomMaterialAsset& out);

// 把 .mat 写回文件(供编辑器通用参数编辑保存)。
bool saveMaterialAsset(const std::string& full_path, const CustomMaterialAsset& asset);

}  // namespace wen