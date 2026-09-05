#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace wen {

// 内置标准 PBR 着色器标识:.mat 的 shader 字段为该值时走延迟通道(MeshPass),
// 其余 shader 值视为自定义着色器,走 forward 通道(CustomMaterialPass)。
constexpr const char* kBuiltinPbrShader = "builtin/pbr";

// 通用材质参数:值统一以 vec4 存储(is_color 决定显示为颜色还是标量)。
// 序列化到 SSBO 的 slot 2+;内置 base_color/mode 在 slot 0-1。
struct CustomMaterialParam {
    std::string name;
    glm::vec4 value = glm::vec4(0.0f);
    bool is_color = false;  // true: vec3 颜色; false: 标量(float 取 value.x)
};

// 统一材质资产(.mat,JSON)。两种形态:
//   标准 PBR(shader == "builtin/pbr"):
//     { "shader":"builtin/pbr", "base_color":[1,1,1], "metallic":0.0, "roughness":0.5,
//       "texture_path":"earth.jpg", "normal_map_path":"", "normal_scale":1.0,
//       "mr_map_path":"", "ao_map_path":"", "ao_intensity":1.0,
//       "emissive_color":[0,0,0], "emissive_intensity":0.0, "tiling":[1,1] }
//   自定义着色器(shader 指向 .frag):
//     { "shader":"custom/unlit.frag", "base_color":[1,0,0], "intensity":1.0, "mode":0,
//       "wireframe":false, "cull":"none", "params":{"color_a":[1,0.2,0.2],"factor":0.5} }
// 字段名与 MaterialComponent 的反射成员保持一致,便于资产/覆写互转。
struct CustomMaterialAsset {
    std::string shader = kBuiltinPbrShader;  // "builtin/pbr" 或相对 shaders 目录的 .frag
    glm::vec3 base_color = glm::vec3(1.0f);

    // ---- 自定义着色器材质字段(shader != builtin/pbr) ----
    float intensity = 1.0f;
    float mode = 0.0f;      // 0 = unlit, 1 = 乘以顶点色
    bool wireframe = false; // true = 用线框(多边形模式 Line)绘制
    std::string cull = "none"; // "none" / "front" / "back" —— 背面剔除
    std::vector<CustomMaterialParam> params;  // 通用参数(上限见 pass 的 slot 容量)

    // ---- 标准 PBR 材质字段(shader == builtin/pbr) ----
    float metallic = 0.0f;
    float roughness = 0.5f;
    std::string texture_path;   // 相对 <资源根>/textures;空 = 无纹理
    std::string normal_map_path;
    float normal_scale = 1.0f;
    std::string mr_map_path;    // metallic-roughness 贴图(B=金属度, G=粗糙度)
    std::string ao_map_path;
    float ao_intensity = 1.0f;
    glm::vec3 emissive_color{0.0f};
    float emissive_intensity = 0.0f;
    glm::vec2 tiling{1.0f, 1.0f};

    bool isBuiltinPbr() const { return shader == kBuiltinPbrShader; }
};

// 从完整文件路径解析 .mat;失败返回 false 并保留 out 默认值。
bool loadMaterialAsset(const std::string& full_path, CustomMaterialAsset& out);

// 把 .mat 写回文件(供编辑器通用参数编辑保存)。
bool saveMaterialAsset(const std::string& full_path, const CustomMaterialAsset& asset);

}  // namespace wen
