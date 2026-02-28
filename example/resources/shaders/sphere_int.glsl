#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include "ray_tracing.glsl"

// sphere data buffer
layout(binding = 3, scalar) buffer SphereDataBuffer {
    Sphere spheres[];
} sphere_data_buffer;

void main() {
    Sphere sphere = sphere_data_buffer.spheres[gl_PrimitiveID];

    vec3 CO = gl_WorldRayOriginEXT - (gl_ObjectToWorldEXT * vec4(sphere.center, 1)).xyz;
    float a = dot(gl_WorldRayDirectionEXT, gl_WorldRayDirectionEXT);
    float b = 2 * dot(gl_WorldRayDirectionEXT, CO);
    float c = dot(CO, CO) - sphere.radius * sphere.radius;
    float delta = b * b - 4 * a * c;

    if (delta >= 0) {
        float sqrt_delta = sqrt(delta);
        if (-b - sqrt_delta > 1e-4) {
            reportIntersectionEXT((-b - sqrt_delta) / (2 * a), 0);
        } else {
            reportIntersectionEXT((-b + sqrt_delta) / (2 * a), 1);
        }
    }
}