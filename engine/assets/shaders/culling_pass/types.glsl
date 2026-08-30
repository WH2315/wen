
struct MeshInstance {
    vec3 location;
    vec3 rotation;
    vec3 scale;
    uint mesh_id;
    vec3 base_color;
    uint texture_index;
    float metallic;
    float roughness;
    vec3 emissive_color;
    float emissive_intensity;
    vec2 tiling;
    uint normal_texture_index;
    float normal_scale;
    uint mr_texture_index;
    uint ao_texture_index;
    float ao_intensity;
};

struct MeshDescriptor {
    uint lod_count;
    uint lods[7];
    vec3 aabb_min;
    float radius;
    vec3 aabb_max;
    float pad1;
};

struct PrimitiveDescriptor {
    int vertex_offset;
    uint first_index;
    uint index_count;
    float pad2;
};

layout(push_constant) uniform Constants {
    uint mesh_instance_count;
    uint primitive_count;
    uvec2 depth_texture_size;
    uint selected_mesh_instance_index;
};

struct VkDrawIndexedIndirectCommand {
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

layout(std430, set = 1, binding = 0) readonly buffer _MeshInstance {
    MeshInstance mesh_instances[];
};

layout(std430, set = 1, binding = 1) readonly buffer _MeshDescriptor {
    MeshDescriptor mesh_descriptors[];
};

layout(std430, set = 1, binding = 2) readonly buffer _PrimitiveDescriptor {
    PrimitiveDescriptor primitive_descriptors[];
};

layout(set = 1, binding = 3) uniform Camera {
    mat4 view;
    mat4 project;
    float near;
    float far;
} camera;
