#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "hit_color.glsl"

// instance address buffer
layout(binding = 5, scalar) buffer InstanceAddressBuffer {
    InstanceAddress addresses[];
} instance_address_buffer;

// custom material data buffer
layout(binding = 6, scalar) buffer MaterialDataBuffer {
    Material materials[];
} material_data_buffer;

// 物体的位置
layout(buffer_reference, scalar) buffer Vertices {
    Vertex vertices[];
};

// 三角形的索引
layout(buffer_reference, scalar) buffer Indices {
    Index indices[];
};

hitAttributeEXT vec3 attribs;

void main() {
    InstanceAddress instance_address = instance_address_buffer.addresses[gl_InstanceCustomIndexEXT];
    Material material = material_data_buffer.materials[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(instance_address.vertex_buffer_address);
    Indices indices = Indices(instance_address.index_buffer_address);

    // 三角形索引
    Index idxs = indices.indices[gl_PrimitiveID];
    // 三角形顶点
    Vertex v0 = vertices.vertices[idxs.i0];
    Vertex v1 = vertices.vertices[idxs.i1];
    Vertex v2 = vertices.vertices[idxs.i2];

    // 质心权重
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // 命中点坐标(模型空间 -> 世界空间)
    vec3 position = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    position = (gl_ObjectToWorldEXT * vec4(position, 1.0)).xyz;
    // 命中点法线(模型空间 -> 世界空间)
    vec3 normal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z);
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0.0)).xyz);

    computeHitColor(position, normal, material);
}