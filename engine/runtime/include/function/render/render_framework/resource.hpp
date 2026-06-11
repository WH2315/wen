#pragma once

#include "function/render/interface/interface.hpp"

namespace wen {

struct Resource {
    // culling pass
    uint32_t depth_mip_level_count;
    std::shared_ptr<Renderer::Sampler> depth_sampler;
    std::vector<std::shared_ptr<Renderer::DepthImage>> depth_images;

    std::shared_ptr<Renderer::InFlightBuffer> counts_buffer;
    std::shared_ptr<Renderer::InFlightBuffer> visible_mesh_instance_indices_buffer;
    std::shared_ptr<Renderer::InFlightBuffer> primitive_counts_buffer;
    std::shared_ptr<Renderer::InFlightBuffer> indirect_commands_buffer;
    std::shared_ptr<Renderer::InFlightBuffer> available_indirect_commands_buffer;
    std::shared_ptr<Renderer::InFlightBuffer> instance_datas_buffer;

    // outlining pass
    uint32_t selected_mesh_instance_index;
};

}  // namespace wen