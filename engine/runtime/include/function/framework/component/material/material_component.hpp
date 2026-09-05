#pragma once

#include "function/framework/component.hpp"
#include "function/asset/material_asset.hpp"
#include "engine/global_context.hpp"
#include <glm/glm.hpp>

namespace wen {

// 统一材质组件(双轨合一的入口):
//  - material_path 为空:直接使用下方内置 PBR 参数(旧行为,不入资产)。
//  - material_path 非空:引用 .mat 资产(相对 <资源根>/materials)。
//      * 资产 shader == builtin/pbr → 延迟通道(MeshPass);内置字段中
//        override_* 为 true / 纹理路径非空 的项覆写资产值,其余用资产值。
//      * 资产 shader 指向自定义 .frag → forward 通道(CustomMaterialPass),
//        此时本组件的 PBR 字段不参与渲染。
// 所有成员变化都会触发 applyToMeshInstance(编辑器/撤销自动同步)。
class MaterialComponent : public Component {
    REFLECT_CLASS("MaterialComponent")

public:
    std::string getClassName() const override { return "MaterialComponent"; }
    static std::string GetClassName() { return "MaterialComponent"; }

    MaterialComponent() : base_color(1.0f), metallic(0.0f), roughness(0.5f) {
        // 编辑器/撤销改成员后自动同步到网格实例。
        addMemberUpdateCallback([this](Component*) { applyToMeshInstance(); });
    }

    // 材质资产路径(相对 <资源根>/materials 的 .mat);空 = 使用内置参数。
    REFLECT_MEMBER()
    std::string material_path;

    // ---- 内置 PBR 参数(material_path 为空时生效;非空时作为覆写来源) ----
    REFLECT_MEMBER()
    glm::vec3 base_color;

    REFLECT_MEMBER()
    float metallic;

    REFLECT_MEMBER()
    float roughness;

    // 纹理路径(相对 <资源根>/textures);空 = 无纹理。
    REFLECT_MEMBER()
    std::string texture_path;

    // 自发光(不参与光照,直接叠加到最终颜色)。
    REFLECT_MEMBER()
    glm::vec3 emissive_color{0.0f};

    REFLECT_MEMBER()
    float emissive_intensity = 0.0f;

    // UV 平铺:纹理坐标重复次数。
    REFLECT_MEMBER()
    glm::vec2 tiling{1.0f, 1.0f};

    // 法线贴图(相对 <资源根>/textures);空 = 无法线贴图。
    REFLECT_MEMBER()
    std::string normal_map_path;

    // 法线贴图强度。
    REFLECT_MEMBER()
    float normal_scale = 1.0f;

    // metallic-roughness 贴图(相对 <资源根>/textures;B=金属度, G=粗糙度)。
    REFLECT_MEMBER()
    std::string mr_map_path;

    // 环境光遮蔽贴图。
    REFLECT_MEMBER()
    std::string ao_map_path;

    // AO 强度(0=关闭)。
    REFLECT_MEMBER()
    float ao_intensity = 1.0f;

    // ---- 覆写开关(material_path 非空时生效;纹理路径非空即视为覆写) ----
    REFLECT_MEMBER()
    bool override_base_color = false;

    REFLECT_MEMBER()
    bool override_metallic = false;

    REFLECT_MEMBER()
    bool override_roughness = false;

    REFLECT_MEMBER()
    bool override_emissive_color = false;

    REFLECT_MEMBER()
    bool override_emissive_intensity = false;

    REFLECT_MEMBER()
    bool override_tiling = false;

    REFLECT_MEMBER()
    bool override_normal_scale = false;

    REFLECT_MEMBER()
    bool override_ao_intensity = false;

    void onCreate() override;
    void onDestroy() override;

    // 是否自定义着色器材质(引用了 shader != builtin/pbr 的 .mat)。
    bool isCustomShaderMaterial() const;

    // 把最终材质参数写入渲染侧网格实例;自定义材质不写(由 CustomMaterialPass 绘制),
    // 并在标准/自定义通道切换时让 MeshComponent 重建,避免双份绘制。
    void applyToMeshInstance();

    // 编辑器设置纹理路径后立即重载(与撤销无关的即时应用)。
    void setTexturePath(const std::string& path) {
        texture_path = path;
        applyToMeshInstance();
    }

    void setNormalMapPath(const std::string& path) {
        normal_map_path = path;
        applyToMeshInstance();
    }

    void setMrMapPath(const std::string& path) {
        mr_map_path = path;
        applyToMeshInstance();
    }

    void setAoMapPath(const std::string& path) {
        ao_map_path = path;
        applyToMeshInstance();
    }

    // .mat 资产的完整路径(material_path 为空时返回空串)。
    std::string materialFullPath() const;

private:
    void writeMeshInstance(const glm::vec3& color, float metal, float rough, uint32_t texture_index,
                           const glm::vec3& emissive_color, float emissive_intensity,
                           const glm::vec2& tiling, uint32_t normal_index, float normal_scale,
                           uint32_t mr_index, uint32_t ao_index, float ao_intensity);

    bool applied_custom_ = false;  // 上次应用时的通道(运行时状态,不序列化)
};

}  // namespace wen
