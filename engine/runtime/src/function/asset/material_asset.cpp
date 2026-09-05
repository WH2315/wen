#include "function/asset/material_asset.hpp"
#include <json.hpp>
#include <fstream>

namespace wen {

namespace {

glm::vec3 jsonVec3(const nlohmann::json& v, const glm::vec3& fallback) {
    if (v.is_array() && v.size() >= 3) {
        return glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    }
    return fallback;
}

glm::vec2 jsonVec2(const nlohmann::json& v, const glm::vec2& fallback) {
    if (v.is_array() && v.size() >= 2) {
        return glm::vec2(v[0].get<float>(), v[1].get<float>());
    }
    return fallback;
}

}  // namespace

bool loadMaterialAsset(const std::string& full_path, CustomMaterialAsset& out) {
    std::ifstream in(full_path);
    if (!in) {
        return false;
    }
    nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }
    out.shader = j.value("shader", out.shader);
    if (j.contains("base_color")) {
        out.base_color = jsonVec3(j["base_color"], out.base_color);
    }

    // 自定义着色器材质字段。
    out.intensity = j.value("intensity", out.intensity);
    out.mode = j.value("mode", out.mode);
    out.wireframe = j.value("wireframe", out.wireframe);
    out.cull = j.value("cull", out.cull);

    out.params.clear();
    if (j.contains("params") && j["params"].is_object()) {
        for (auto it = j["params"].begin(); it != j["params"].end(); ++it) {
            CustomMaterialParam param;
            param.name = it.key();
            if (it.value().is_array()) {
                param.is_color = true;
                param.value = glm::vec4(jsonVec3(it.value(), glm::vec3(0.0f)), 1.0f);
            } else if (it.value().is_number()) {
                param.is_color = false;
                param.value = glm::vec4(it.value().get<float>(), 0.0f, 0.0f, 0.0f);
            } else {
                continue;
            }
            out.params.push_back(std::move(param));
        }
    }

    // 标准 PBR 材质字段(字段名与 MaterialComponent 反射成员一致)。
    out.metallic = j.value("metallic", out.metallic);
    out.roughness = j.value("roughness", out.roughness);
    out.texture_path = j.value("texture_path", out.texture_path);
    out.normal_map_path = j.value("normal_map_path", out.normal_map_path);
    out.normal_scale = j.value("normal_scale", out.normal_scale);
    out.mr_map_path = j.value("mr_map_path", out.mr_map_path);
    out.ao_map_path = j.value("ao_map_path", out.ao_map_path);
    out.ao_intensity = j.value("ao_intensity", out.ao_intensity);
    if (j.contains("emissive_color")) {
        out.emissive_color = jsonVec3(j["emissive_color"], out.emissive_color);
    }
    out.emissive_intensity = j.value("emissive_intensity", out.emissive_intensity);
    if (j.contains("tiling")) {
        out.tiling = jsonVec2(j["tiling"], out.tiling);
    }
    return true;
}

bool saveMaterialAsset(const std::string& full_path, const CustomMaterialAsset& asset) {
    nlohmann::json j;
    j["shader"] = asset.shader;
    j["base_color"] = {asset.base_color.x, asset.base_color.y, asset.base_color.z};

    if (asset.isBuiltinPbr()) {
        // 标准 PBR:只写标准字段,避免把自定义字段混入内置材质文件。
        j["metallic"] = asset.metallic;
        j["roughness"] = asset.roughness;
        j["texture_path"] = asset.texture_path;
        j["normal_map_path"] = asset.normal_map_path;
        j["normal_scale"] = asset.normal_scale;
        j["mr_map_path"] = asset.mr_map_path;
        j["ao_map_path"] = asset.ao_map_path;
        j["ao_intensity"] = asset.ao_intensity;
        j["emissive_color"] = {asset.emissive_color.x, asset.emissive_color.y, asset.emissive_color.z};
        j["emissive_intensity"] = asset.emissive_intensity;
        j["tiling"] = {asset.tiling.x, asset.tiling.y};
    } else {
        // 自定义着色器:写自定义字段。
        j["intensity"] = asset.intensity;
        j["mode"] = asset.mode;
        j["wireframe"] = asset.wireframe;
        j["cull"] = asset.cull;
        nlohmann::json params = nlohmann::json::object();
        for (const auto& param : asset.params) {
            if (param.is_color) {
                params[param.name] = {param.value.x, param.value.y, param.value.z};
            } else {
                params[param.name] = param.value.x;
            }
        }
        if (!params.empty()) {
            j["params"] = std::move(params);
        }
    }

    std::ofstream out(full_path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << j.dump(2);
    return true;
}

}  // namespace wen
