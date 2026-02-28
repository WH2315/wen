#pragma once

#include "function/render/interface/resource/image.hpp"
#include "function/render/interface/resource/buffer.hpp"
#include "core/base/macro.hpp"
#include <tiny_gltf.h>
#include <glm/gtc/quaternion.hpp>

namespace wen::Renderer {

struct ModelBLASInfo {
    std::unique_ptr<StorageBuffer> buffer;
    vk::AccelerationStructureKHR blas = nullptr;
    ~ModelBLASInfo();
};

class Model {
public:
    enum class ModelType {
        eNormalModel,
        eGLTFPrimitive,
        eSphereModel,
    };

    virtual ModelType getType() const = 0;

    virtual ~Model() {
        if (blas_info.has_value()) {
            blas_info.reset();
        }
    }

    std::optional<std::unique_ptr<ModelBLASInfo>> blas_info{std::nullopt};
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && color == other.color;
    }
};

struct Offset {
    uint32_t vertex;
    uint32_t index;
};

class Mesh final {
public:
    Mesh();
    ~Mesh();

    Offset offset;
    std::vector<uint32_t> indices;
};

class NormalModel : public Model {
public:
    NormalModel(const std::string& filename, const std::vector<std::string>& blacklist = {});
    ~NormalModel() override;

    uint32_t vertex_count;
    uint32_t index_count;

    auto vertices() const { return vertices_; }

    auto meshes() const { return meshes_; }

    auto offset() const { return offset_; }

    Offset upload(std::shared_ptr<VertexBuffer> vertex_buffer, std::shared_ptr<IndexBuffer> index_buffer, Offset offset = {0, 0});

    ModelType getType() const override { return ModelType::eNormalModel; }

    std::unique_ptr<Buffer> ray_tracing_vertex_buffer;
    std::unique_ptr<Buffer> ray_tracing_index_buffer;

private:
    std::vector<Vertex> vertices_;
    std::map<std::string, std::shared_ptr<Mesh>> meshes_;
    Offset offset_;
};

class GLTFScene;

class GLTFPrimitive : public Model {
    friend class RayTracingInstance;

public:
    struct GLTFPrimitiveData {
        uint32_t first_index = 0;
        uint32_t first_vertex = 0;
        uint32_t material_index = 0;
    };

    GLTFPrimitive(GLTFScene& scene, const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::vector<std::string>& attrs);
    ~GLTFPrimitive() override = default;

    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    ModelType getType() const override { return ModelType::eGLTFPrimitive; }

    auto& getData() { return data_; }

private:
    GLTFScene& scene_;
    GLTFPrimitiveData data_;
    glm::vec3 min_;
    glm::vec3 max_;
};

struct GLTFMesh {
    std::vector<std::shared_ptr<GLTFPrimitive>> primitives = {};

    GLTFMesh(GLTFScene& scene, const tinygltf::Model& model, const tinygltf::Mesh& mesh, const std::vector<std::string>& attrs) {
        WEN_CORE_DEBUG("GLTF: load mesh: {}", mesh.name)
        for (auto& primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                WEN_CORE_WARN("GLTF: only triangle mode is supported, skipping primitive")
                continue;
            }
            if (primitive.indices <= -1) {
                WEN_CORE_WARN("GLTF: primitive has no indices, skipping primitive")
                continue;
            }
            primitives.push_back(std::make_shared<GLTFPrimitive>(scene, model, primitive, attrs));
        }
    }

    ~GLTFMesh() { primitives.clear(); }
};

class GLTFNode {
public:
    GLTFNode(GLTFScene& scene, const tinygltf::Model& model, uint32_t index, GLTFNode* parent);
    ~GLTFNode();

    auto getMesh() { return mesh_; }

    glm::mat4 getLocalMatrix();
    glm::mat4 getWorldMatrix();

private:
    std::shared_ptr<GLTFMesh> mesh_ = {};

    GLTFNode* parent_ = {};
    std::vector<std::shared_ptr<GLTFNode>> children_ = {};

    glm::quat rotation_ = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale_ = glm::vec3(1.0f);
    glm::vec3 translation_ = glm::vec3(0.0f);
    glm::mat4 matrix_ = glm::mat4(1.0f);
};

struct GLTFMaterial {
    glm::vec4 base_color_factor = glm::vec4(1.0f);
    int base_color_texture = -1;
    glm::vec3 emissive_factor = glm::vec3(0.0f);
    int emissive_texture = -1;
    int normal_texture = -1;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    int metallic_roughness_texture = -1;
};

class DescriptorSet;

class GLTFScene {
    friend class GLTFNode;
    friend class GLTFPrimitive;

public:
    GLTFScene(const std::string& filename, const std::vector<std::string>& attrs);
    ~GLTFScene();

    void build(std::function<void(GLTFNode*, std::shared_ptr<GLTFPrimitive>)> func);
    void bindTexturesSamplers(const std::shared_ptr<DescriptorSet>& descriptor_set, uint32_t binding);

    auto getMaterialBuffer() { return material_buffer_; }

    auto getAttrBuffer(const std::string& name) { return attr_buffers_.at(name); }

    uint32_t getTexturesCount() { return textures_.size(); }

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    std::unique_ptr<Buffer> ray_tracing_vertex_buffer;
    std::unique_ptr<Buffer> ray_tracing_index_buffer;

private:
    void loadImages(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model);
    void loadMeshesAndPrimitives(const tinygltf::Model& model, const std::vector<std::string>& attrs);
    void loadAttributes();
    void loadNodes(const tinygltf::Model& model);

private:
    std::string filepath_;

    // textures
    std::vector<std::shared_ptr<SpecificTexture>> textures_;
    std::shared_ptr<Sampler> sampler_;

    // materials
    std::vector<GLTFMaterial> materials_;
    std::shared_ptr<StorageBuffer> material_buffer_;

    // meshes
    std::vector<std::shared_ptr<GLTFMesh>> meshes_;

    // attr
    std::map<std::string, std::vector<uint8_t>> attr_datas_;
    std::map<std::string, std::shared_ptr<StorageBuffer>> attr_buffers_;

    // nodes
    std::vector<std::shared_ptr<GLTFNode>> nodes_;
    std::vector<GLTFNode*> nodes_ptr_;
};

using ClassHashCode = decltype(typeid(int).hash_code());
template <class T>
ClassHashCode getClassHashCode() {
    return typeid(std::remove_reference_t<T>).hash_code();
}

template <class CustomDataCreateInfo>
struct CustomDataRegister {
    struct CustomDataWrapper {
        uint32_t size = 0;
        std::function<void*()> create;
        std::function<void(void*)> destroy;
        std::function<void*(void*)> getData;
        std::function<uint32_t(void*)> getSize;
        std::function<void(void*, uint32_t)> alignSize;
        std::function<std::shared_ptr<StorageBuffer>(uint32_t)> createStorageBuffer;
    };

    struct GroupInfo {
        uint32_t offset = 0;
        uint32_t count = 0;
        std::vector<CustomDataCreateInfo> custom_data_cis;
        std::map<ClassHashCode, void*> custom_data_map;
    };

    CustomDataRegister() = default;
    ~CustomDataRegister() {
        custom_data_wrapper_map.clear();
        groups.clear();
        custom_data_buffer_map.clear();
    }

    GroupInfo createGroup() {
        std::map<ClassHashCode, void*> custom_data_map;
        for (auto& [hash_code, wrapper] : custom_data_wrapper_map) {
            custom_data_map.insert(std::make_pair(hash_code, wrapper.create()));
        }
        return {
            .offset = 0,
            .count = 0,
            .custom_data_cis = {},
            .custom_data_map = std::move(custom_data_map),
        };
    }

    uint32_t buildGroup() {
        uint32_t count = 0;
        for (auto& [group_id, group_info] : groups) {
            auto [hash_code, ptr] = *group_info.custom_data_map.begin();
            group_info.offset = count;
            group_info.count = custom_data_wrapper_map.at(hash_code).getSize(ptr);
            count += group_info.count;
        }

        for (auto& [hash_code, wrapper] : custom_data_wrapper_map) {
            custom_data_buffer_map.insert(std::make_pair(hash_code, wrapper.createStorageBuffer(count)));
        }

        for (auto& [group_id, group_info] : groups) {
            for (auto& [hash_code, ptr] : group_info.custom_data_map) {
                auto& wrapper = custom_data_wrapper_map.at(hash_code);
                memcpy(
                    static_cast<uint8_t*>(custom_data_buffer_map.at(hash_code)->map()) + wrapper.size * group_info.offset,
                    wrapper.getData(ptr),
                    wrapper.getSize(ptr) * wrapper.size
                );
                custom_data_buffer_map.at(hash_code)->unmap();
            }
        }

        for (auto& [group_id, group_info] : groups) {
            for (auto& [hash_code, ptr] : group_info.custom_data_map) {
                custom_data_wrapper_map.at(hash_code).destroy(ptr);
            }
            group_info.custom_data_map.clear();
        }

        return count;
    }

    template <class CustomData>
    void registerCustomData() {
        auto hash_code = getClassHashCode<CustomData>();
        using type_t = std::remove_reference_t<CustomData>;
        using vector_t = std::vector<type_t>;
        if (custom_data_wrapper_map.find(hash_code) != custom_data_wrapper_map.end()) {
            WEN_CORE_ERROR("CustomDataRegister: CustomData {} has been registered", typeid(type_t).name())
            return;
        }
        WEN_CORE_DEBUG("CustomDataRegister: Register CustomData {}, hash_code {}", typeid(type_t).name(), hash_code)
        custom_data_wrapper_map[hash_code] = {
            .size = sizeof(type_t),
            .create = [] {
                auto* ptr = new vector_t();
                return static_cast<void*>(ptr);
            },
            .destroy = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                data_ptr->clear();
                delete data_ptr;
            },
            .getData = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                return static_cast<void*>(data_ptr->data());
            },
            .getSize = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                return static_cast<uint32_t>(data_ptr->size());
            },
            .alignSize = [](void* ptr, uint32_t size) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                while (data_ptr->size() < size) {
                    data_ptr->emplace_back();
                }
            },
            .createStorageBuffer = [](uint32_t count) {
                return std::make_shared<StorageBuffer>(
                    sizeof(type_t) * count,
                    vk::BufferUsageFlagBits::eStorageBuffer,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                );
            }
        };
    }

    template <class... CustomDatas>
    void addInstance(uint32_t id, const CustomDataCreateInfo& ci, CustomDatas&&... datas) {
        if (groups.find(id) == groups.end()) {
            groups.insert(std::make_pair(id, createGroup()));
        }

        auto& group_info = groups.at(id);
        group_info.custom_data_cis.emplace_back(ci);

        if constexpr (sizeof...(datas) > 0) {
            addCustomData<CustomDatas...>(group_info, std::forward<CustomDatas>(datas)...);
        }

        for (auto& [hash_code, ptr] : group_info.custom_data_map) {
            custom_data_wrapper_map.at(hash_code).alignSize(ptr, group_info.custom_data_cis.size());
        }
    }

    template <class T, class... Args>
    uint32_t addCustomData(GroupInfo& group, T&& arg, Args&&... args) {
        auto hash = getClassHashCode<T>();
        auto it = group.custom_data_map.find(hash);
        if (it == group.custom_data_map.end() || it->second == nullptr) {
            WEN_CORE_ERROR("CustomDataRegister: custom data ({}) not registered", hash)
            return 0;
        }

        auto* ptr = static_cast<std::vector<std::remove_reference_t<T>>*>(it->second);
        ptr->push_back(std::forward<std::remove_reference_t<T>>(arg));
        if constexpr (sizeof...(args) > 0) {
            return addCustomData<Args...>(group, std::forward<Args>(args)...);
        } else {
            return ptr->size();
        }
    }

    template <class CustomData>
    std::shared_ptr<StorageBuffer> getCustomDataBuffer() {
        return custom_data_buffer_map.at(getClassHashCode<CustomData>());
    }

    void multiThreadUpdate(uint32_t id, const std::function<void(uint32_t, uint32_t, uint32_t)>& update) {
        auto group = groups.at(id);
        uint32_t begin = group.offset;
        uint32_t end = begin + group.count;
        
        uint32_t index = 0;
        std::vector<std::thread> threads;
        while (begin < end) {
            uint32_t batch_end = std::min(begin + 250, end);
            threads.emplace_back([=]() {
                update(index, begin, batch_end);
            });
            index += 250;
            begin = batch_end;
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    std::map<uint32_t, GroupInfo> groups;
    std::map<ClassHashCode, CustomDataWrapper> custom_data_wrapper_map;
    std::map<ClassHashCode, std::shared_ptr<StorageBuffer>> custom_data_buffer_map;
};

class AccelerationStructure;

class SphereModel final : public Model {
    friend class AccelerationStructure;

public:
    struct SphereData {
        glm::vec3 center;
        float radius;
    };

    struct SphereAABBData {
        glm::vec3 min;
        glm::vec3 max;
    };

    SphereModel();
    ~SphereModel() override;

    template <class CustomSphereData>
    void registerCustomSphereData() {
        register_->registerCustomData<CustomSphereData>();
    }

    template <class ...CustomSphereDatas>
    void addSphereModel(uint32_t id, glm::vec3 center, float radius, CustomSphereDatas&&... datas) {
        register_->addInstance(
            id,
            0,
            SphereData{
                .center = center,
                .radius = radius
            },
            std::forward<CustomSphereDatas>(datas)...
        );
    }

    template <class CustomSphereData>
    std::shared_ptr<StorageBuffer> getCustomSphereDataBuffer() {
        return register_->getCustomDataBuffer<CustomSphereData>();
    }
    auto getSphereDataBuffer() {
        return getCustomSphereDataBuffer<SphereData>();
    }

    void build();

    template <class CustomShpereDatas>
    using FunUpdateCustomData = std::function<void(uint32_t, CustomShpereDatas&)>;
    void update(uint32_t id, FunUpdateCustomData<SphereData> callback);

    template <class CustomSphereData>
    void update(uint32_t id, FunUpdateCustomData<CustomSphereData> callback) {
        register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
            auto* ptr = static_cast<CustomSphereData*>(getCustomSphereDataBuffer<CustomSphereData>()->map());
            ptr += begin;
            while (begin < end) {
                callback(index, *ptr);
                ptr++;
                index++;
                begin++;
            }
        });
    }

    ModelType getType() const override { return ModelType::eSphereModel; }

private:
    std::unique_ptr<CustomDataRegister<uint8_t>> register_;
    std::unique_ptr<AccelerationStructure> as_;
    std::vector<SphereAABBData> aabbs_;
    std::shared_ptr<StorageBuffer> aabbs_buffer_;
};

}  // namespace wen::Renderer