#include "function/asset/asset_system.hpp"
#include "core/base/macro.hpp"
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace wen {

struct ObjVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coord;
    glm::vec3 color;

    bool operator==(const ObjVertex& other) const {
        return position == other.position && normal == other.normal &&
               tex_coord == other.tex_coord && color == other.color;
    }
};

}  // namespace wen

namespace std {
template <>
struct hash<wen::ObjVertex> {
    size_t operator()(const wen::ObjVertex& vertex) const {
        return ((((hash<glm::vec3>()(vertex.position) ^
                   (hash<glm::vec3>()(vertex.normal) << 1)) >>
                  1) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};
}  // namespace std

namespace wen {

AssetSystem::AssetSystem() {
    mesh_pool_ = std::make_unique<MeshPool>(
        128 * 1024 * 1024,
        128 * 1024 * 1024,
        getMaxMeshCount(),
        getMaxPrimitiveCount()
    );
}

AssetSystem::~AssetSystem() {
    mesh_pool_.reset();
}

MeshID AssetSystem::loadMesh(const std::string& filename, const std::vector<std::string>& lods) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (path_ + "/models/" + filename).c_str())) {
        WEN_CORE_ERROR("Failed to load mesh: {}", filename)
        return MeshID(-1);
    }

    std::vector<size_t> lods_shape_index;
    if (lods.empty()) {
        size_t lod_index = 0;
        for (const auto& shape : shapes) {
            WEN_CORE_INFO("Auto Selecet {} as LOD {}", shape.name, lod_index)
            lods_shape_index.push_back(lod_index);
            lod_index++;
        }
    } else {
        size_t lod_index = 0;
        size_t last_found_index = 0;
        for (auto lod_shape_name : lods) {
            bool found = false;
            size_t lod_shape_index = 0;
            for (const auto& shape : shapes) {
                if (shape.name == lod_shape_name) {
                    WEN_CORE_INFO("Find {} as LOD {}", shape.name, lod_index)
                    lods_shape_index.push_back(lod_shape_index);
                    last_found_index = lod_shape_index;
                    found = true;
                    break;
                }
                lod_shape_index++;
            }
            if (!found) {
                WEN_CORE_INFO("Auto Selecet {} as LOD {}", shapes[last_found_index].name, lod_index)
                lods_shape_index.push_back(last_found_index);
            }
            lod_index++;
        }
    }

    MeshData data{};
    for (auto lod_shape_index : lods_shape_index) {
        auto& primitive = data.lods.emplace_back();
        std::unordered_map<ObjVertex, uint32_t> unique_vertices;
        for (const auto& index : shapes[lod_shape_index].mesh.indices) {
            ObjVertex vertex{};
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            if (attrib.texcoords.empty()) {
                vertex.tex_coord = {0, 0};
            } else {
                vertex.tex_coord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1],
                };
            }
            if (attrib.colors.empty()) {
                vertex.color = {1, 1, 1};
            } else {
                vertex.color = {
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2],
                };
            }

            float c = pow((float)(data.lods.size() - 1) / max_level_of_details, 0.8);
            vertex.color = {c, 0.7 - std::abs(0.5 - c), 1 - c};

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices.insert(std::make_pair(vertex, primitive.positions.size()));
                primitive.positions.push_back(vertex.position);
                primitive.normals.push_back(vertex.normal);
                primitive.tex_coords.push_back(vertex.tex_coord);
                primitive.colors.push_back(vertex.color);
            }
            primitive.indices.push_back(unique_vertices.at(vertex));
        }
    }

    return mesh_pool_->uploadMeshData(data);
}

}  // namespace wen