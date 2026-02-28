#include "function/render/interface/resource/descriptor_set.hpp"
#include "function/render/interface/context.hpp"
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace std {

template <>
struct hash<wen::Renderer::Vertex> {
    size_t operator()(wen::Renderer::Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec3>()(vertex.color) << 1);
    }
};

}  // namespace std

namespace wen::Renderer {

ModelBLASInfo::~ModelBLASInfo() {
    manager->device->device.destroyAccelerationStructureKHR(blas, nullptr, manager->dispatcher);
    buffer.reset();
}

Mesh::Mesh() {}

Mesh::~Mesh() { indices.clear(); }

NormalModel::NormalModel(const std::string& filename, const std::vector<std::string>& blacklist)
    : vertex_count(0), index_count(0) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        WEN_CORE_ERROR("Failed to load model: {0}", filename)
        throw std::runtime_error(warn + err);
    }

    for (const auto& name : blacklist) {
        auto it = shapes.begin();
        while (it != shapes.end()) {
            if (it->name == name) {
                it = shapes.erase(it);
                break;
            }
            ++it;
        }
    }

    uint32_t size = 0;
    for (const auto& shape : shapes) {
        size += shape.mesh.indices.size();
    }
    vertices_.reserve(size);

    std::unordered_map<Vertex, uint32_t> unique_vertices = {};
    unique_vertices.reserve(size);

    for (const auto& shape : shapes) {
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};
            if (index.normal_index < 0) {
                vertex.normal = {0.0f, 0.0f, 0.0f};
            } else {
                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
            }
            vertex.color = {1.0f, 1.0f, 1.0f};

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices.insert(std::make_pair(vertex, vertices_.size()));
                vertices_.push_back(vertex);
            }
            mesh->indices.push_back(unique_vertices[vertex]);
        }
        index_count += mesh->indices.size();
        meshes_.insert(std::make_pair(shape.name, std::move(mesh)));
    }
    vertex_count = vertices_.size();
}

Offset NormalModel::upload(std::shared_ptr<VertexBuffer> vertex_buffer, std::shared_ptr<IndexBuffer> index_buffer, Offset offset) {
    offset_ = offset;
    auto temp = offset;
    temp.vertex = vertex_buffer->setData(vertices_, temp.vertex);
    for (auto& [name, mesh] : meshes_) {
        mesh->offset.vertex = offset_.vertex;
        mesh->offset.index = temp.index;
        temp.index = index_buffer->setData(mesh->indices, temp.index);
    }
    return temp;
}

NormalModel::~NormalModel() {
    vertices_.clear();
    meshes_.clear();
    ray_tracing_vertex_buffer.reset();
    ray_tracing_index_buffer.reset();
}

GLTFNode::GLTFNode(GLTFScene& scene, const tinygltf::Model& model, uint32_t index, GLTFNode* parent)
    : parent_(parent) {
    auto& node = model.nodes[index];

    if (node.mesh > -1) {
        mesh_ = scene.meshes_[node.mesh];
    }
    if (node.rotation.size() == 4) {
        auto rotate = glm::make_quat(node.rotation.data());
        rotation_.x = rotate.w;
        rotation_.y = rotate.x;
        rotation_.z = rotate.y;
        rotation_.w = rotate.z;
    }
    if (node.scale.size() == 3) {
        scale_ = glm::make_vec3(node.scale.data());
    }
    if (node.translation.size() == 3) {
        translation_ = glm::make_vec3(node.translation.data());
    }
    if (node.matrix.size() == 16) {
        matrix_ = glm::make_mat4(node.matrix.data());
    }

    for (auto child_index : node.children) {
        auto child_node = std::make_shared<GLTFNode>(scene, model, child_index, this);
        scene.nodes_ptr_.push_back(child_node.get());
        children_.push_back(std::move(child_node));
    }
}

glm::mat4 GLTFNode::getLocalMatrix() {
    return glm::translate(glm::mat4(1.0f), translation_) * glm::mat4(rotation_) * glm::scale(glm::mat4(1.0f), scale_) * matrix_;
}

glm::mat4 GLTFNode::getWorldMatrix() {
    if (parent_ != nullptr) {
        return parent_->getWorldMatrix() * getLocalMatrix();
    } else {
        return getLocalMatrix();
    }
}

GLTFNode::~GLTFNode() { children_.clear(); }

GLTFPrimitive::GLTFPrimitive(GLTFScene& scene, const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::vector<std::string>& attrs)
    : scene_(scene) {
    data_.first_vertex = scene.vertices.size();
    data_.first_index = scene.indices.size();
    data_.material_index = primitive.material;

    auto getAccessorData = [&](const std::string& name, auto fun) {
        if (primitive.attributes.find(name) == primitive.attributes.end()) {
            WEN_CORE_WARN("primitive has no attribute {}", name)
            return static_cast<const void*>(nullptr);
        }
        const auto& accessor = model.accessors[primitive.attributes.find(name)->second];
        const auto& buffer_view = model.bufferViews[accessor.bufferView];
        fun(accessor);
        return reinterpret_cast<const void*>(model.buffers[buffer_view.buffer].data.data() + buffer_view.byteOffset + accessor.byteOffset);
    };
    auto* positionPtr = static_cast<const float*>(
        getAccessorData("POSITION", [&](const auto& accessor) {
            vertex_count = accessor.count;
            min_ = glm::make_vec3(accessor.minValues.data());
            max_ = glm::make_vec3(accessor.maxValues.data());
        }));

    for (uint32_t i = 0; i < vertex_count; i++) {
        scene.vertices.push_back(glm::make_vec3(&positionPtr[i * 3]));
    }
    for (auto attr : attrs) {
        uint32_t count = 0;
        uint32_t stride = 0;
        auto* ptr = static_cast<const uint8_t*>(
            getAccessorData(attr, [&](auto& accessor) {
                count = accessor.count;
                switch (accessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                        stride = sizeof(double);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    case TINYGLTF_COMPONENT_TYPE_INT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        stride = sizeof(uint32_t);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_SHORT:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        stride = sizeof(uint16_t);
                        break;
                    case TINYGLTF_COMPONENT_TYPE_BYTE:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        stride = sizeof(uint8_t);
                        break;
                }
                switch (accessor.type) {
                    case TINYGLTF_TYPE_VEC2:
                        stride *= 2;
                        break;
                    case TINYGLTF_TYPE_VEC3:
                        stride *= 3;
                        break;
                    case TINYGLTF_TYPE_VEC4:
                        stride *= 4;
                        break;
                    case TINYGLTF_TYPE_MAT2:
                        stride *= 4;
                        break;
                    case TINYGLTF_TYPE_MAT3:
                        stride *= 9;
                        break;
                    case TINYGLTF_TYPE_MAT4:
                        stride *= 16;
                        break;
                    default:
                        WEN_CORE_WARN("unsupported accessor.type {} {} {}", accessor.type, __FILE__, __LINE__)
                }
            }));
        if (scene.attr_datas_.find(attr) == scene.attr_datas_.end()) {
            scene.attr_datas_[attr] = {};
        }
        WEN_CORE_DEBUG("{}: count {}, stride {}", attr, count, stride)
        auto& data = scene.attr_datas_[attr];
        auto start = data.size();
        data.resize(start + count * stride);
        memcpy(data.data() + start, ptr, count * stride);
    }

    const auto& index_accessor = model.accessors[primitive.indices];
    const auto& index_buffer_view = model.bufferViews[index_accessor.bufferView];
    index_count = index_accessor.count;
    auto* index_ptr = static_cast<const uint8_t*>(model.buffers[index_buffer_view.buffer].data.data() + index_buffer_view.byteOffset + index_accessor.byteOffset);
    switch (index_accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (uint32_t i = 0; i < index_count; i++) {
                scene.indices.push_back(*(reinterpret_cast<const uint32_t*>(index_ptr)));
                index_ptr += 4;
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (uint32_t i = 0; i < index_count; i++) {
                scene.indices.push_back(*(reinterpret_cast<const uint16_t*>(index_ptr)));
                index_ptr += 2;
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (uint32_t i = 0; i < index_count; i++) {
                scene.indices.push_back(*(reinterpret_cast<const uint8_t*>(index_ptr)));
                index_ptr += 1;
            }
            break;
    }
}

GLTFScene::GLTFScene(const std::string& filename, const std::vector<std::string>& attrs) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    size_t pos = filename.find_last_of('/');
    filepath_ = filename.substr(0, pos);

    auto filetype = filename.substr(filename.find_last_of('.') + 1);
    bool ret = false;
    if (filetype == "gltf") {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    } else if (filetype == "glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else {
        WEN_CORE_ERROR("unknown GLTF filetype {}", filename)
    }

    if (!warn.empty()) {
        WEN_CORE_WARN("warn: {} {}", warn, filename)
    }
    if (!err.empty()) {
        WEN_CORE_ERROR("err: {} {}", err, filename)
    }
    if (!ret) {
        WEN_CORE_FATAL("failed to load GLTF {}", filename)
        return;
    }

    for (auto& extension : model.extensionsRequired) {
        WEN_CORE_DEBUG("GLTF model required \"{}\" extension", extension)
    }

    loadImages(model);
    loadMaterials(model);
    loadMeshesAndPrimitives(model, attrs);
    loadAttributes();
    loadNodes(model);
}

void GLTFScene::build(std::function<void(GLTFNode*, std::shared_ptr<GLTFPrimitive>)> fun) {
    for (auto* node : nodes_ptr_) {
        if (node->getMesh() == nullptr) {
            continue;
        }
        for (auto& primitive : node->getMesh()->primitives) {
            fun(node, primitive);
        }
    }
}

void GLTFScene::bindTexturesSamplers(const std::shared_ptr<DescriptorSet>& descriptor_set, uint32_t binding) {
    std::vector<std::pair<std::shared_ptr<SpecificTexture>, std::shared_ptr<Sampler>>> textures_samplers;
    textures_samplers.reserve(textures_.size());
    for (auto& texture : textures_) {
        textures_samplers.emplace_back(texture, sampler_);
    }
    descriptor_set->bindTextures(binding, textures_samplers);
}

void GLTFScene::loadImages(const tinygltf::Model& model) {
    for (auto& image : model.images) {
        if (image.uri.empty()) {
            WEN_CORE_WARN("unsupported image format {}", image.name)
            continue;
        }

        std::string::size_type pos;
        if ((pos = image.uri.find_last_of('.')) != std::string::npos) {
            if (image.uri.substr(pos + 1) == "ktx") {
                // TODO:
                WEN_CORE_ERROR("")
                continue;
            }
        }

        std::vector<uint8_t> rgba;
        const uint8_t* data = nullptr;
        if (image.component == 3) {
            rgba.resize(image.width * image.height * 4);
            auto* rgb = image.image.data();
            auto* ptr = rgba.data();
            for (uint32_t i = 0; i < image.width * image.height; i++) {
                ptr[0] = rgb[0];
                ptr[1] = rgb[1];
                ptr[2] = rgb[2];
                ptr[3] = 0;
                ptr += 4;
                rgb += 3;
            }
            data = rgba.data();
            WEN_CORE_DEBUG("convert 3 channel(RGB) image to 4(RGBA) channel image {} X {}", image.width, image.height)
        } else {
            assert(image.component == 4);
            data = image.image.data();
            WEN_CORE_DEBUG("use 4 channel(RGBA) image {} X {}", image.width, image.height)
        }
        textures_.push_back(std::make_shared<DataTexture>(data, image.width, image.height, 0));
    }
    sampler_ = std::make_shared<Sampler>(SamplerOptions{});
}

void GLTFScene::loadMaterials(const tinygltf::Model& model) {
    for (auto& material : model.materials) {
        auto& mat = materials_.emplace_back();
        mat.base_color_factor = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
        mat.base_color_texture = material.pbrMetallicRoughness.baseColorTexture.index;
        mat.emissive_factor = glm::make_vec3(material.emissiveFactor.data());
        mat.emissive_texture = material.emissiveTexture.index;
        mat.normal_texture = material.normalTexture.index;
        mat.metallic_factor = material.pbrMetallicRoughness.metallicFactor;
        mat.roughness_factor = material.pbrMetallicRoughness.roughnessFactor;
        mat.metallic_roughness_texture = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
    }
    material_buffer_ = std::make_shared<StorageBuffer>(
        materials_.size() * sizeof(GLTFMaterial),
        vk::BufferUsageFlags{},
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    auto* ptr = static_cast<uint8_t*>(material_buffer_->map());
    memcpy(ptr, materials_.data(), material_buffer_->getSize());
    material_buffer_->unmap();
}

void GLTFScene::loadMeshesAndPrimitives(const tinygltf::Model& model, const std::vector<std::string>& attrs) {
    for (const auto& mesh : model.meshes) {
        meshes_.push_back(std::make_shared<GLTFMesh>(*this, model, mesh, attrs));
    }
}

void GLTFScene::loadAttributes() {
    for (auto& [name, data] : attr_datas_) {
        attr_buffers_[name] = std::make_shared<StorageBuffer>(
            data.size(),
            vk::BufferUsageFlags{},
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        );
        auto* ptr = static_cast<uint8_t*>(attr_buffers_[name]->map());
        memcpy(ptr, data.data(), attr_buffers_[name]->getSize());
        attr_buffers_[name]->unmap();
    }
}

void GLTFScene::loadNodes(const tinygltf::Model& model) {
    auto default_scene = model.scenes[std::max(0, model.defaultScene)];
    for (auto index : default_scene.nodes) {
        auto node = std::make_shared<GLTFNode>(*this, model, index, nullptr);
        nodes_ptr_.push_back(node.get());
        nodes_.push_back(std::move(node));
    }
}

GLTFScene::~GLTFScene() {
    nodes_.clear();
    nodes_ptr_.clear();
    attr_datas_.clear();
    attr_buffers_.clear();
    meshes_.clear();
    materials_.clear();
    material_buffer_.reset();
    textures_.clear();
    sampler_.reset();
    vertices.clear();
    indices.clear();
    ray_tracing_vertex_buffer.reset();
    ray_tracing_index_buffer.reset();
}

SphereModel::SphereModel() {
    register_ = std::make_unique<CustomDataRegister<uint8_t>>();
    register_->registerCustomData<SphereData>();
    as_ = std::make_unique<AccelerationStructure>();
}

SphereModel::~SphereModel() {
    register_.reset();
}

void SphereModel::build() {
    auto count = register_->buildGroup();
    if (count == 0) {
        WEN_CORE_ERROR("Not add any sphere model")
    }
    aabbs_.reserve(count);
    auto* ptr = static_cast<SphereData*>(getSphereDataBuffer()->map());
    for (uint32_t i = 0; i < count; ++i) {
        aabbs_.push_back({
            .min = ptr->center - ptr->radius,
            .max = ptr->center + ptr->radius,
        });
        ++ptr;
    }
    getSphereDataBuffer()->unmap();
    aabbs_buffer_ = std::make_shared<StorageBuffer>(
        aabbs_.size() * sizeof(SphereAABBData),
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    memcpy(aabbs_buffer_->map(), aabbs_.data(), aabbs_buffer_->getSize());
    aabbs_buffer_->unmap();
}

void SphereModel::update(uint32_t id, FunUpdateCustomData<SphereData> callback) {
    register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
            auto* sphere_data_ptr = static_cast<SphereData*>(register_->getCustomDataBuffer<SphereData>()->map());
            auto* aabb_data_ptr = static_cast<SphereAABBData*>(aabbs_buffer_->map());
            aabb_data_ptr += begin;
            sphere_data_ptr += begin;
            while (begin < end) {
                callback(index, *sphere_data_ptr);
                aabb_data_ptr->min = sphere_data_ptr->center - sphere_data_ptr->radius;
                aabb_data_ptr->max = sphere_data_ptr->center + sphere_data_ptr->radius;
                sphere_data_ptr++;
                aabb_data_ptr++;
                index++;
                begin++;
            }
        });
    std::shared_ptr<SphereModel> self(this, [](auto) {});
    as_->addModel(self);
    as_->build(true, true);
}

}  // namespace wen::Renderer