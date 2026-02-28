#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "ray_tracing.glsl"

layout(location = 0) rayPayloadInEXT Ray ray;

const vec3 sky = vec3(0.8, 1.0, 1.0);
const vec3 sun = vec3(500);
const vec3 sun_dir = normalize(vec3(0.4, 1.0, 0.5));

void main() {
    ray.count += 1000;
    if (dot(ray.direction, sun_dir) > 0.997) {
        ray.color += ray.albedo * sun;
    } else {
        ray.color += ray.albedo * sky;
    }
}