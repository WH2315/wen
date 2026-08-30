#include "function/asset/asset_system.hpp"
#include "core/base/macro.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <functional>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace wen {

struct ObjVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    glm::vec3 color;
    glm::vec3 tangent;

    bool operator==(const ObjVertex& other) const {
        return position == other.position && normal == other.normal &&
               texcoord == other.texcoord && color == other.color && tangent == other.tangent;
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
               (hash<glm::vec2>()(vertex.texcoord) << 1) ^
               (hash<glm::vec3>()(vertex.tangent) << 2);
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
    texture_pool_ = std::make_unique<TexturePool>(64);
    normal_texture_pool_ = std::make_unique<TexturePool>(64);
    mr_texture_pool_ = std::make_unique<TexturePool>(64);
    ao_texture_pool_ = std::make_unique<TexturePool>(64);
}

AssetSystem::~AssetSystem() {
    ao_texture_pool_.reset();
    mr_texture_pool_.reset();
    normal_texture_pool_.reset();
    texture_pool_.reset();
    mesh_pool_.reset();
}

MeshID AssetSystem::loadMesh(const std::string& filename, const std::vector<std::string>& lods) {
    if (auto iter = loaded_meshes_.find(filename); iter != loaded_meshes_.end()) {
        return iter->second;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path_ + "/models/" + filename,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords |
        aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
    );
    if (scene == nullptr || scene->mRootNode == nullptr) {
        WEN_CORE_ERROR("Failed to load mesh: {}", filename)
        return MeshID(-1);
    }

    // 按节点(OBJ 中的 object/group)收集形状候选,保持树序
    std::vector<std::pair<std::string, std::vector<const aiMesh*>>> shapes;
    std::function<void(aiNode*)> collect = [&](aiNode* node) {
        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            collect(node->mChildren[i]);
        }
        if (node->mNumMeshes == 0) {
            return;
        }
        std::vector<const aiMesh*> meshes;
        meshes.reserve(node->mNumMeshes);
        for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
            meshes.push_back(scene->mMeshes[node->mMeshes[i]]);
        }
        shapes.emplace_back(node->mName.C_Str(), std::move(meshes));
    };
    collect(scene->mRootNode);

    std::vector<size_t> lods_shape_index;
    if (lods.empty()) {
        size_t lod_index = 0;
        for (const auto& [name, _] : shapes) {
            WEN_CORE_INFO("Auto Select {} as LOD {}", name, lod_index)
            lods_shape_index.push_back(lod_index);
            lod_index++;
        }
    } else {
        size_t lod_index = 0;
        size_t last_found_index = 0;
        for (auto lod_shape_name : lods) {
            bool found = false;
            size_t lod_shape_index = 0;
            for (const auto& [name, _] : shapes) {
                if (name == lod_shape_name) {
                    WEN_CORE_INFO("Find {} as LOD {}", name, lod_index)
                    lods_shape_index.push_back(lod_shape_index);
                    last_found_index = lod_shape_index;
                    found = true;
                    break;
                }
                lod_shape_index++;
            }
            if (!found) {
                WEN_CORE_INFO("Auto Select {} as LOD {}", shapes[last_found_index].first, lod_index)
                lods_shape_index.push_back(last_found_index);
            }
            lod_index++;
        }
    }

    MeshData data{};
    for (auto lod_shape_index : lods_shape_index) {
        auto& primitive = data.lods.emplace_back();
        std::unordered_map<ObjVertex, uint32_t> unique_vertices;
        for (const aiMesh* ai_mesh : shapes[lod_shape_index].second) {
            for (uint32_t f = 0; f < ai_mesh->mNumFaces; f++) {
                const aiFace& face = ai_mesh->mFaces[f];
                for (uint32_t v = 0; v < face.mNumIndices; v++) {
                    uint32_t index = face.mIndices[v];
                    ObjVertex vertex{};
                    vertex.position = {ai_mesh->mVertices[index].x, ai_mesh->mVertices[index].y, ai_mesh->mVertices[index].z};
                    if (ai_mesh->HasNormals()) {
                        vertex.normal = {ai_mesh->mNormals[index].x, ai_mesh->mNormals[index].y, ai_mesh->mNormals[index].z};
                    } else {
                        vertex.normal = {0, 0, 0};
                    }
                    if (ai_mesh->HasTextureCoords(0)) {
                        vertex.texcoord = {ai_mesh->mTextureCoords[0][index].x, ai_mesh->mTextureCoords[0][index].y};
                    } else {
                        vertex.texcoord = {0, 0};
                    }
                    if (ai_mesh->HasVertexColors(0)) {
                        vertex.color = {ai_mesh->mColors[0][index].r, ai_mesh->mColors[0][index].g, ai_mesh->mColors[0][index].b};
                    } else {
                        vertex.color = {1, 1, 1};
                    }
                    if (ai_mesh->HasTangentsAndBitangents()) {
                        vertex.tangent = {ai_mesh->mTangents[index].x, ai_mesh->mTangents[index].y, ai_mesh->mTangents[index].z};
                    } else {
                        vertex.tangent = {1, 0, 0};
                    }

                    // 默认顶点颜色取网格自身颜色(无顶点色的 OBJ 为白色)。
                    // 注意:这里曾用基于 LOD 层级的调试色覆盖(c=0 时 {0,0.2,1}=蓝色),
                    // 已移除 —— 网格默认应显示 base_color(默认白),而非 LOD 调试色。
                    if (unique_vertices.count(vertex) == 0) {
                        unique_vertices.insert(std::make_pair(vertex, primitive.positions.size()));
                        primitive.positions.push_back(vertex.position);
                        primitive.normals.push_back(vertex.normal);
                        primitive.texcoords.push_back(vertex.texcoord);
                        primitive.colors.push_back(vertex.color);
                        primitive.tangents.push_back(vertex.tangent);
                    }
                    primitive.indices.push_back(unique_vertices.at(vertex));
                }
            }
        }
    }

    auto mesh_id = mesh_pool_->uploadMeshData(data);
    loaded_meshes_.insert({filename, mesh_id});
    return mesh_id;
}

}  // namespace wen