#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "random.glsl"
#include "ray_tracing.glsl"

// camera
layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 project;
} camera;

// rt_instance
layout(binding = 1) uniform accelerationStructureEXT rt_instance;

// output image
layout(binding = 2, rgba32f) uniform image2D image;

layout(push_constant) uniform PushConstant {
    float time;
    int frame_index;
    int sample_count;
    float view_depth;
    float view_depth_strength;
    int max_ray_recursion_depth;
} constant;

layout(location = 0) rayPayloadEXT Ray ray;

void main() {
    vec3 color = vec3(0.0);
    ray.state = uint((constant.time + 1) * gl_LaunchIDEXT.x * gl_LaunchIDEXT.y);     

    for (int i = 0; i < constant.sample_count; ++i) {
        vec2 center = vec2(gl_LaunchIDEXT.xy) + vec2(rnd(ray.state), rnd(ray.state));
        center /= vec2(gl_LaunchSizeEXT.xy);
        center.y = 1 - center.y;
        center = center * 2 - 1; // [-1, 1]

        mat4 inverse_view = inverse(camera.view);
        vec3 camera_view_point = (inverse(camera.project) * vec4(center, -1, 1)).xyz;
        camera_view_point *= constant.view_depth / -camera_view_point.z;
        vec3 world_camera_view_point = (inverse_view * vec4(camera_view_point, 1)).xyz;
        vec2 camera_random_point = vec2(
            constant.view_depth_strength * (rnd(ray.state) - 0.5),
            constant.view_depth_strength * (rnd(ray.state) - 0.5)
        );
        vec3 world_camera_random_point = (inverse_view * vec4(camera_random_point, 0, 1)).xyz;

        ray.count = 0;
        ray.color = vec3(0);
        ray.albedo = vec3(1);
        ray.origin = world_camera_random_point;
        ray.direction = normalize(world_camera_view_point - world_camera_random_point);

        while (ray.count < constant.max_ray_recursion_depth) {
            traceRayEXT(
                rt_instance,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0,
                0,
                0,
                ray.origin,
                0.001,
                ray.direction,
                10000.0,
                0
            );
        }

        color += ray.color;
    }

    float scale = 1.0f / float(constant.frame_index + 1);
    vec3 old_color = imageLoad(image, ivec2(gl_LaunchIDEXT.xy)).xyz;
    color /= float(constant.sample_count);
    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(mix(old_color, color, scale), 1));
}