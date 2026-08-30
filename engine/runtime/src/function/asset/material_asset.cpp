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
    return true;
}

bool saveMaterialAsset(const std::string& full_path, const CustomMaterialAsset& asset) {
    nlohmann::json j;
    j["shader"] = asset.shader;
    j["base_color"] = {asset.base_color.x, asset.base_color.y, asset.base_color.z};
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

    std::ofstream out(full_path, std::ios::trunc);
    if (!out) {
        return false;
    }
    out << j.dump(2);
    return true;
}

}  // namespace wen