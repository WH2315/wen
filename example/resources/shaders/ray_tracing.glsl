#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct Ray {
    int count;
    uint state;
    vec3 color;
    vec3 albedo;
    vec3 origin;
    vec3 direction;
};

struct InstanceAddress {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 color;
};

struct Index {
    uint i0;
    uint i1;
    uint i2;
};

struct GLTFPrimitiveData {
    uint first_index;
    uint first_vertex;
    uint material_index;
};

struct GLTFMaterial {
    vec4 base_color_factor;
    int base_color_texture;
    vec3 emissive_factor;
    int emissive_texture;
    int normal_texture;
    float metallic_factor;
    float roughness_factor;
    int metallic_roughness_texture;
};

struct Sphere {
    vec3 center;
    float radius;
};