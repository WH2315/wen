#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "random.glsl"
#include "ray_tracing.glsl"

// instance address buffer
layout(binding = 5, scalar) readonly buffer InstanceAddressBuffer {
    InstanceAddress addresses[];
} instance_address_buffer;

// GLTF: primitive data buffer
layout(binding = 7, scalar) readonly buffer PrimitiveDataBuffer {
    GLTFPrimitiveData primitives[];
} primitive_data_buffer;

// material buffer
layout(binding = 8, scalar) readonly buffer MaterialBuffer {
    GLTFMaterial materials[];
} material_buffer;

// NORMAL
layout(binding = 9, scalar) readonly buffer NormalBuffer {
    vec3 normals[];
} normal_buffer;

// TEXCOORD_0
layout(binding = 10, scalar) readonly buffer Texcoord0Buffer {
    vec2 texcoords0[];
} texcoord0_buffer;

// all textures
layout(binding = 11) uniform sampler2D textures[];

// 物体的位置
layout(buffer_reference, scalar) buffer Vertices {
    vec3 vertices[];
};

// 三角形的索引
layout(buffer_reference, scalar) buffer Indices {
    Index indices[];
};

layout(location = 0) rayPayloadInEXT Ray ray;
hitAttributeEXT vec3 attribs;

const float PI = 3.1415926535;

// 反射强度函数
vec3 F(vec3 F0, float cos_theta) {
    return F0 + (1 - F0) * pow(clamp(1 - cos_theta, 0.0, 1.0), 5.0);
}

// 法线分布函数
float NDF_GGX(vec3 N, vec3 M, float roughness2) {
    float NoM = dot(N, M);
    if (NoM <= 0) {
        return 0;
    }
    float n = 1 + NoM * NoM * (roughness2 - 1);
    return clamp(roughness2 / (PI * n * n + 0.001), 0, 1);
}

// 几何遮挡函数
// G/4(NoV)(NoL)
float G2(float NoL, float NoV, float roughness2) {
    float lambda_in = NoV * sqrt(roughness2 + NoL * sqrt(NoL - roughness2 * NoL));
    float lambda_out = NoL * sqrt(roughness2 + NoV * sqrt(NoV - roughness2 * NoV));
    return 0.5 / (lambda_in + lambda_out + 0.001);
}

vec3 BRDF(vec3 direction, vec3 albedo, vec3 N, vec3 V, vec3 F0, float roughness2, float metallic) {
    vec3 H = normalize(V + direction);      // 半程向量
    float NoL = max(dot(N, direction), 0);  // 入射角
    float NoV = max(dot(N, V), 0);          // 视角
    float LoH = max(dot(direction, H), 0);

    // 漫反射
    vec3 diffuse_brdf = albedo * (1 - metallic) / PI;
    // 镜面反射
    vec3 spec_brdf = F(F0, LoH) * G2(NoL, NoV, roughness2) * NDF_GGX(N, H, roughness2);

    return (diffuse_brdf + spec_brdf) * NoL;   
}

void main() {
    InstanceAddress instance_address = instance_address_buffer.addresses[gl_InstanceCustomIndexEXT];
    GLTFPrimitiveData primitive_data = primitive_data_buffer.primitives[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(instance_address.vertex_buffer_address);
    Indices indices = Indices(instance_address.index_buffer_address);

    GLTFMaterial material = material_buffer.materials[primitive_data.material_index];
    Index idxs = indices.indices[primitive_data.first_index / 3 + gl_PrimitiveID];
    idxs.i0 += primitive_data.first_vertex;
    idxs.i1 += primitive_data.first_vertex;
    idxs.i2 += primitive_data.first_vertex;
    const vec3 v0 = vertices.vertices[idxs.i0];
    const vec3 v1 = vertices.vertices[idxs.i1];
    const vec3 v2 = vertices.vertices[idxs.i2];
    const vec3 n0 = normal_buffer.normals[idxs.i0];
    const vec3 n1 = normal_buffer.normals[idxs.i1];
    const vec3 n2 = normal_buffer.normals[idxs.i2];
    const vec2 uv0 = texcoord0_buffer.texcoords0[idxs.i0];
    const vec2 uv1 = texcoord0_buffer.texcoords0[idxs.i1];
    const vec2 uv2 = texcoord0_buffer.texcoords0[idxs.i2];

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 position = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
    position = (gl_ObjectToWorldEXT * vec4(position, 1.0)).xyz;
    vec3 normal = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0.0)).xyz);
    vec2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

    // 获取材质属性
    // 基础颜色
    vec3 albedo = material.base_color_factor.rgb;
    if (material.base_color_texture > -1) {
        // 有一个有效的基础颜色纹理, 使用 uv 坐标来采样纹理的颜色值
        albedo *= texture(textures[nonuniformEXT(material.base_color_texture)], uv).rgb;
    }

    // 金属度, 粗糙度
    float metallic = material.metallic_factor;
    float roughness = material.roughness_factor;
    if (material.metallic_roughness_texture > -1) {
        vec4 rm = texture(textures[nonuniformEXT(material.metallic_roughness_texture)], uv);
        metallic *= rm.r;
        roughness *= rm.b;
    }

    // 自发光
    vec3 emissive = material.emissive_factor;
    if (material.emissive_texture > -1) {
        emissive *= texture(textures[nonuniformEXT(material.emissive_texture)], uv).rgb;
    }

    ray.count += 1;
    // 光线的反照率
    ray.albedo *= albedo;

    float r = sqrt(rnd(ray.state));
    float phi = 2 * PI * rnd(ray.state);
    vec3 cosine_direction_local = vec3(r * cos(phi), sqrt(1 - r * r), r * sin(phi));
    vec3 y_axis = normal;
    vec3 up = abs(y_axis.y) < 0.999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
    vec3 x_axis = normalize(cross(up, y_axis));
    vec3 z_axis = normalize(cross(x_axis, y_axis));
    vec3 cosine_direction = normalize(
        cosine_direction_local.x * x_axis +
        cosine_direction_local.y * y_axis +
        cosine_direction_local.z * z_axis
    );
    float cosine_pdf = dot(cosine_direction, normal) / PI;

    vec3 reflect_direction = reflect(ray.direction, normal);
    float reflect_pdf = 1;

    ray.direction = normalize(mix(
        reflect_direction,
        cosine_direction,
        roughness
    ));

    float pdf = mix(
        reflect_pdf,
        cosine_pdf,
        roughness
    );

    ray.color += ray.albedo * emissive;

    vec3 brdf_cosine = BRDF(
        ray.direction,
        albedo,
        normal,
        normalize(ray.origin - position),
        albedo * metallic + (vec3(0.04) * (1 - metallic)),
        roughness * roughness,
        metallic
    );
    ray.albedo *= brdf_cosine / pdf;
    ray.origin = position;
}