#include "function/render/interface/resource/ray_tracing.hpp"
#include "function/render/interface/basic/utils.hpp"
#include "function/render/interface/context.hpp"

namespace wen::Renderer {

AccelerationStructure::~AccelerationStructure() {
    staging_.reset();
    scratch_.reset();
}

void AccelerationStructure::addModel(std::shared_ptr<Model> model) {
    auto type = model->getType();
    switch (type) {
        case Model::ModelType::eNormalModel:
            models_.emplace_back(std::dynamic_pointer_cast<NormalModel>(model));
            break;
        case Model::ModelType::eGLTFPrimitive:
            WEN_CORE_ERROR("GLTFPrimitive is used to addGLTFScene!")
            break;
        case Model::ModelType::eSphereModel:
            sphere_models_.emplace_back(std::dynamic_pointer_cast<SphereModel>(model));
            break;
    }
}

void AccelerationStructure::addGLTFScene(std::shared_ptr<GLTFScene> scene) {
    scenes_.emplace_back(scene);
}

void AccelerationStructure::build(bool is_update, bool allow_update) {
    std::vector<AccelerationStructureInfo> infos;
    infos.reserve(models_.size() + scenes_.size() + sphere_models_.size());

    uint64_t max_staging_size = current_staging_size_, max_scratch_size = current_scratch_size_;
    auto mode = is_update ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;
    auto flags = vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (allow_update) {
        flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    }

    for (auto& model : models_) {
        if (!is_update) {
            uint64_t vertices_size = model->vertex_count * sizeof(Vertex);
            uint64_t indices_size = model->index_count * sizeof(uint32_t);
            model->ray_tracing_vertex_buffer = std::make_unique<Buffer>(
                vertices_size,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            );
            model->ray_tracing_index_buffer = std::make_unique<Buffer>(
                indices_size,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            );
            max_staging_size = std::max(max_staging_size, vertices_size);
            max_staging_size = std::max(max_staging_size, indices_size);
            model->blas_info = std::make_unique<ModelBLASInfo>();
        }
        auto& as_info = infos.emplace_back(*(model->blas_info.value()));
        uint32_t max_primitive_count = model->index_count / 3;

        as_info.geometries.emplace_back()
            .triangles
            .setVertexFormat(vk::Format::eR32G32B32Sfloat)
            .setVertexData(getBufferAddress(model->ray_tracing_vertex_buffer->buffer))
            .setVertexStride(sizeof(Vertex))
            .setIndexType(vk::IndexType::eUint32)
            .setIndexData(getBufferAddress(model->ray_tracing_index_buffer->buffer))
            .setMaxVertex(model->vertex_count - 1)
            .sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        as_info.as_geometries.emplace_back()
            .setGeometry(as_info.geometries.back())
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        as_info.build_range_infos.emplace_back()
            .setFirstVertex(0)
            .setPrimitiveCount(max_primitive_count)
            .setPrimitiveOffset(0)
            .setTransformOffset(0);
        as_info.build_info
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(mode)
            .setFlags(flags)
            .setGeometries(as_info.as_geometries);
        as_info.size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            as_info.build_info,
            max_primitive_count,
            manager->dispatcher
        );
        max_scratch_size = std::max(max_scratch_size, is_update ? as_info.size_info.updateScratchSize : as_info.size_info.buildScratchSize);
    }
    for (auto& scene : scenes_) {
        if (!is_update) {
            uint64_t vertices_size = scene->vertices.size() * sizeof(glm::vec3);
            uint64_t indices_size = scene->indices.size() * sizeof(uint32_t);
            scene->ray_tracing_vertex_buffer = std::make_unique<Buffer>(
                vertices_size,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            );
            scene->ray_tracing_index_buffer = std::make_unique<Buffer>(
                indices_size,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            );
            max_staging_size = std::max(max_staging_size, vertices_size);
            max_staging_size = std::max(max_staging_size, indices_size);
        }
        scene->build([&](auto* node, auto primitive) {
            if (!is_update) {
                primitive->blas_info = std::make_unique<ModelBLASInfo>();
            }
            auto& as_info = infos.emplace_back(*primitive->blas_info.value());
            uint32_t max_primitive_count = primitive->index_count / 3;

            as_info.geometries.emplace_back()
                .triangles
                .setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexData(getBufferAddress(scene->ray_tracing_vertex_buffer->buffer))
                .setVertexStride(sizeof(glm::vec3))
                .setIndexType(vk::IndexType::eUint32)
                .setIndexData(getBufferAddress(scene->ray_tracing_index_buffer->buffer))
                .setMaxVertex(primitive->vertex_count - 1)
                .sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
            as_info.as_geometries.emplace_back()
                .setGeometry(as_info.geometries.back())
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
            as_info.build_range_infos.emplace_back()
                .setFirstVertex(primitive->getData().first_vertex)
                .setPrimitiveCount(max_primitive_count)
                .setPrimitiveOffset(primitive->getData().first_index * sizeof(uint32_t))
                .setTransformOffset(0);
            as_info.build_info
                .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                .setMode(mode)
                .setFlags(flags)
                .setGeometries(as_info.as_geometries);
            as_info.size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                as_info.build_info,
                max_primitive_count,
                manager->dispatcher
            );
            max_scratch_size = std::max(max_scratch_size, is_update ? as_info.size_info.updateScratchSize : as_info.size_info.buildScratchSize);
        });
    }
    for (auto& model : sphere_models_) {
        if (!is_update) {
            if (model->aabbs_.empty()) {
                model->build();
            }
            model->blas_info = std::make_unique<ModelBLASInfo>();
        }
        auto max_primitive_count = static_cast<uint32_t>(model->aabbs_.size());
        auto& as_info = infos.emplace_back(*(model->blas_info.value()));

        as_info.geometries.emplace_back()
            .aabbs
            .setData(getBufferAddress(model->aabbs_buffer_->getBuffer()))
            .setStride(sizeof(SphereModel::SphereAABBData))
            .sType = vk::StructureType::eAccelerationStructureGeometryAabbsDataKHR;
        as_info.as_geometries.emplace_back()
            .setGeometry(as_info.geometries.back())
            .setGeometryType(vk::GeometryTypeKHR::eAabbs)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        as_info.build_range_infos.emplace_back()
            .setFirstVertex(0)
            .setPrimitiveOffset(0)
            .setPrimitiveCount(max_primitive_count)
            .setTransformOffset(0);
        as_info.build_info
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(mode)
            .setFlags(flags)
            .setGeometries(as_info.as_geometries);
        as_info.size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            as_info.build_info,
            max_primitive_count,
            manager->dispatcher
        );
        max_scratch_size = std::max(max_scratch_size, is_update ? as_info.size_info.updateScratchSize : as_info.size_info.buildScratchSize);
    }

    if (max_staging_size > current_staging_size_) {
        staging_.reset();
        staging_ = std::make_unique<StorageBuffer>(
            max_staging_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
        );
        current_staging_size_ = max_staging_size;
    }
    if (max_scratch_size > current_scratch_size_) {
        scratch_.reset();
        scratch_ = std::make_unique<StorageBuffer>(
            max_scratch_size,
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        );
        current_scratch_size_ = max_scratch_size;
    }

    if (!is_update) {
        for (auto& model : models_) {
            auto* ptr = static_cast<uint8_t*>(staging_->map());

            uint64_t vertices_size = model->vertex_count * sizeof(Vertex);
            std::vector<Vertex> rt_vertices;
            rt_vertices.reserve(model->vertex_count);
            for (const auto& v : model->vertices()) {
                rt_vertices.push_back({v.position, v.normal, v.color});
            }
            memcpy(ptr, rt_vertices.data(), vertices_size);
            staging_->flush(vertices_size, model->ray_tracing_vertex_buffer->buffer);

            uint64_t indices_size = 0;
            for (const auto& [name, mesh] : model->meshes()) {
                uint64_t size = mesh->indices.size() * sizeof(uint32_t);
                memcpy(ptr, mesh->indices.data(), size);
                ptr += size;
                indices_size += size;
            }
            staging_->flush(indices_size, model->ray_tracing_index_buffer->buffer);
        }
        for (auto& scene : scenes_) {
            auto* ptr = static_cast<uint8_t*>(staging_->map());

            uint64_t vertices_size = scene->vertices.size() * sizeof(glm::vec3);
            memcpy(ptr, scene->vertices.data(), vertices_size);
            staging_->flush(vertices_size, scene->ray_tracing_vertex_buffer->buffer);

            uint64_t indices_size = scene->indices.size() * sizeof(uint32_t);
            memcpy(ptr, scene->indices.data(), indices_size);
            staging_->flush(indices_size, scene->ray_tracing_index_buffer->buffer);
        }
        if (staging_ != nullptr) {
            staging_->unmap();
        }
    }

    vk::QueryPoolCreateInfo query_pool_ci{};
    query_pool_ci.setQueryCount(static_cast<uint32_t>(infos.size()))
        .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
    auto query_pool = manager->device->device.createQueryPool(query_pool_ci);

    std::vector<uint32_t> indices;
    vk::DeviceSize batch_size = {0};
    vk::DeviceSize batch_limit = {256 * 1024 * 1024}; // 256MB
    for (int i = 0; i < infos.size(); ++i) {
        indices.push_back(i);
        batch_size += infos[i].size_info.accelerationStructureSize;
        if (batch_size > batch_limit || i == infos.size() - 1) {
            auto cmdbuf = manager->command_pool->allocateSingleUse();
            cmdbuf.resetQueryPool(query_pool, 0, infos.size());
            if (!is_update) {
                uint32_t index = 0;
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    as_info.buffer = std::make_unique<StorageBuffer>(
                        as_info.size_info.accelerationStructureSize,
                        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                    );
                    vk::AccelerationStructureCreateInfoKHR as_ci{};
                    as_ci.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(as_info.buffer->getBuffer())
                        .setSize(as_info.size_info.accelerationStructureSize);
                    as_info.blas = manager->device->device.createAccelerationStructureKHR(as_ci, nullptr, manager->dispatcher);
                    as_info.build_info
                        .setDstAccelerationStructure(as_info.blas)
                        .setScratchData(getBufferAddress(scratch_->getBuffer()));
                    cmdbuf.buildAccelerationStructuresKHR(as_info.build_info, as_info.build_range_infos.data(), manager->dispatcher);

                    vk::MemoryBarrier barrier{};
                    barrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                        .setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);
                    cmdbuf.pipelineBarrier(
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::DependencyFlags{},
                        barrier,
                        {},
                        {}
                    );
                    cmdbuf.writeAccelerationStructuresPropertiesKHR(
                        as_info.blas,
                        vk::QueryType::eAccelerationStructureCompactedSizeKHR,
                        query_pool,
                        index,
                        manager->dispatcher
                    );
                    index++;
                }
                manager->command_pool->freeSingleUse(cmdbuf);

                std::vector<vk::DeviceSize> compacted_sizes(indices.size());
                auto result = manager->device->device.getQueryPoolResults(
                    query_pool,
                    0,
                    static_cast<uint32_t>(indices.size()),
                    sizeof(vk::DeviceSize) * compacted_sizes.size(),
                    compacted_sizes.data(),
                    sizeof(vk::DeviceSize),
                    vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64
                );
                if (result != vk::Result::eSuccess) {
                    WEN_CORE_ERROR("Failed to get query pool results.")
                }

                cmdbuf = manager->command_pool->allocateSingleUse();
                index = 0;
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    auto& model_blas_info = as_info.model_blas_info;
                    model_blas_info.buffer = std::make_unique<StorageBuffer>(
                        compacted_sizes[index],
                        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
                    );

                    vk::AccelerationStructureCreateInfoKHR as_ci{};
                    as_ci.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(model_blas_info.buffer->getBuffer())
                        .setSize(compacted_sizes[index]);
                    model_blas_info.blas = manager->device->device.createAccelerationStructureKHR(as_ci, nullptr, manager->dispatcher);

                    vk::CopyAccelerationStructureInfoKHR compact{};
                    compact.setSrc(as_info.blas)
                        .setDst(model_blas_info.blas)
                        .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
                    cmdbuf.copyAccelerationStructureKHR(compact, manager->dispatcher);
                    index++;
                }
            } else {
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    auto& model_blas_info = as_info.model_blas_info;
                    as_info.build_info
                        .setSrcAccelerationStructure(model_blas_info.blas)
                        .setDstAccelerationStructure(model_blas_info.blas)
                        .setScratchData(getBufferAddress(scratch_->getBuffer()));
                    cmdbuf.buildAccelerationStructuresKHR(as_info.build_info, as_info.build_range_infos.data(), manager->dispatcher);

                    vk::MemoryBarrier barrier{};
                    barrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                        .setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);
                    cmdbuf.pipelineBarrier(
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::DependencyFlags{},
                        barrier,
                        {},
                        {}
                    );
                }
            }
            manager->command_pool->freeSingleUse(cmdbuf);

            batch_size = 0;
            indices.clear();
        }
    }

    if (!is_update) {
        for (auto& as_info : infos) {
            manager->device->device.destroyAccelerationStructureKHR(as_info.blas, nullptr, manager->dispatcher);
            as_info.buffer.reset();
        }
    }
    infos.clear();
    manager->device->device.destroyQueryPool(query_pool);
    models_.clear();
    scenes_.clear();
    sphere_models_.clear();
}

RayTracingInstance::RayTracingInstance() : instance_count_(0) {
    register_ = std::make_unique<CustomDataRegister<InstanceCreateInfo>>();
    register_->registerCustomData<InstanceAddress>();
    register_->registerCustomData<GLTFPrimitive::GLTFPrimitiveData>();
}

RayTracingInstance::~RayTracingInstance() {
    instance_buffer_.reset();
    manager->device->device.destroyAccelerationStructureKHR(tlas_, nullptr, manager->dispatcher);
    buffer_.reset();
    scratch_.reset();
}

void RayTracingInstance::build(bool allow_update) {
    allow_update_ = allow_update;
    instance_count_ = register_->buildGroup();
    instance_buffer_ = std::make_unique<Buffer>(
        instance_count_ * sizeof(vk::AccelerationStructureInstanceKHR),
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    auto* ptr = static_cast<vk::AccelerationStructureInstanceKHR*>(instance_buffer_->map());
    for (auto& [id, group] : register_->groups) {
        uint32_t index = group.offset;
        auto* instance_ptr = ptr + index;
        for (auto& [binding, model, transform] : group.custom_data_cis) {
            instance_ptr->setInstanceCustomIndex(index)
                .setAccelerationStructureReference(getAccelerationStructureAddress(model->blas_info.value()->blas))
                .setInstanceShaderBindingTableRecordOffset(binding)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setMask(0xff)
                .setTransform(convert<vk::TransformMatrixKHR, const glm::mat4&>(transform));
            instance_ptr++;
            index++;
        }
        group.custom_data_cis.clear();
    }

    vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances{};
    geometry_instances.setData(getBufferAddress(instance_buffer_->buffer));
    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.setGeometry(geometry_instances)
        .setGeometryType(vk::GeometryTypeKHR::eInstances);

    vk::AccelerationStructureBuildGeometryInfoKHR build{};
    vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (allow_update) {
        flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    }
    build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setFlags(flags)
        .setGeometries(geometry);

    auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        build,
        instance_count_,
        manager->dispatcher
    );

    vk::AccelerationStructureCreateInfoKHR tlas_ci{};
    buffer_ = std::make_unique<StorageBuffer>(
        size_info.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    tlas_ci.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setSize(size_info.accelerationStructureSize)
        .setBuffer(buffer_->getBuffer());
    tlas_ = manager->device->device.createAccelerationStructureKHR(tlas_ci, nullptr, manager->dispatcher);

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    scratch_ = std::make_unique<StorageBuffer>(
        size_info.buildScratchSize,
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    build.setDstAccelerationStructure(tlas_)
        .setScratchData(getBufferAddress(scratch_->getBuffer()));
    vk::AccelerationStructureBuildRangeInfoKHR range{};
    range.setPrimitiveCount(instance_count_)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);
    cmdbuf.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void RayTracingInstance::update(uint32_t id, FunUpdateInstance callback) {
    if (!allow_update_) {
        WEN_CORE_WARN("you had set allow_update to false, so you can't update the instance!")
        return;
    }

    register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
        auto* instance_ptr = static_cast<vk::AccelerationStructureInstanceKHR*>(instance_buffer_->data);
        instance_ptr += begin;
        FunUpdateTransform update_transform = [&](const glm::mat4& transform) {
            instance_ptr->setTransform(convert<vk::TransformMatrixKHR, const glm::mat4&>(transform));
        };
        FunUpdataAccelerationStructure update_as = [&](std::shared_ptr<Model> model) {
            instance_ptr->setAccelerationStructureReference(getAccelerationStructureAddress(model->blas_info.value()->blas));
        };
        while (begin < end) {
            callback(index, update_transform, update_as);
            instance_ptr++;
            index++;
            begin++;
        }
    });

    vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances{};
    geometry_instances.setData(getBufferAddress(instance_buffer_->buffer));
    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.setGeometry(geometry_instances)
        .setGeometryType(vk::GeometryTypeKHR::eInstances);

    vk::AccelerationStructureBuildGeometryInfoKHR build{};
    build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setGeometries(geometry);

    auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        build,
        instance_count_,
        manager->dispatcher
    );

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    if (scratch_->getSize() < size_info.updateScratchSize) {
        scratch_.reset();
        scratch_ = std::make_unique<StorageBuffer>(
            size_info.updateScratchSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_GPU_ONLY,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
        );
    }
    build.setSrcAccelerationStructure(tlas_)
        .setDstAccelerationStructure(tlas_)
        .setScratchData(getBufferAddress(scratch_->getBuffer()));
    vk::AccelerationStructureBuildRangeInfoKHR range{};
    range.setPrimitiveCount(instance_count_)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);
    cmdbuf.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
    manager->command_pool->freeSingleUse(cmdbuf);
}

InstanceAddress RayTracingInstance::createInstanceAddress(Model& model) {
    switch (model.getType()) {
        case Model::ModelType::eNormalModel:
            return {
                getBufferAddress(dynamic_cast<NormalModel&>(model).ray_tracing_vertex_buffer->buffer),
                getBufferAddress(dynamic_cast<NormalModel&>(model).ray_tracing_index_buffer->buffer),
            };
        case Model::ModelType::eGLTFPrimitive:
            return {
                getBufferAddress(dynamic_cast<GLTFPrimitive&>(model).scene_.ray_tracing_vertex_buffer->buffer),
                getBufferAddress(dynamic_cast<GLTFPrimitive&>(model).scene_.ray_tracing_index_buffer->buffer),
            };
        case Model::ModelType::eSphereModel:
            return {
                0,
                0,
            };
    }
    return {};
}

}  // namespace wen::Renderer