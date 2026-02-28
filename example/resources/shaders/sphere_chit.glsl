#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include "hit_color.glsl"

// sphere data buffer
layout(binding = 3, scalar) buffer SphereDataBuffer {
    Sphere spheres[];
} sphere_data_buffer;

// custom sphere material data buffer
layout(binding = 4, scalar) buffer MaterialDataBuffer {
    Material materials[];
} sphere_material_data_buffer;

const float PI = 3.1415926535;

void main() {
    Sphere sphere = sphere_data_buffer.spheres[gl_PrimitiveID];
    Material material = sphere_material_data_buffer.materials[gl_PrimitiveID];

    // 把球心从物体局部空间变换到世界空间
    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;
    // o + td，得到世界空间的命中位置 
    vec3 position = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    // 球面法线
    vec3 normal = normalize(position - center);

    if (sphere.radius > 100) {
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

        if (mod(floor(position.x * 8), 8) == 0 || mod(floor(position.z * 8), 8) == 0) {
            ray.albedo *= 0.05;
        }
        ray.albedo *= material.albedo;
        ray.origin = position;
        ray.direction = cosine_direction;
    } else {
        computeHitColor(position, normal, material); 
    }
}