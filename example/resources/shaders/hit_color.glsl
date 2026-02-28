#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "random.glsl"
#include "ray_tracing.glsl"

struct Material {
    vec3 albedo;
    float roughness;
    vec3 specular_albedo;
    float specular_probability;
    vec3 emissive_color;
    float emissive_intensity;
};

layout(location = 0) rayPayloadInEXT Ray ray;

void computeHitColor(vec3 position, vec3 normal, Material material) {
    ray.count += 1;
    float is_specular = float(material.specular_probability >= rnd(ray.state)); 
    ray.color += ray.albedo * material.emissive_color * material.emissive_intensity;
    ray.albedo *= mix(material.albedo, material.specular_albedo, is_specular);

    vec3 random_direction = normalize(vec3(uniform_rnd(ray.state, 0, 1), uniform_rnd(ray.state, 0, 1), uniform_rnd(ray.state, 0, 1)));
    vec3 diff_direction = normalize(normal + random_direction);
    ray.direction = normalize(mix(
        reflect(gl_WorldRayDirectionEXT, normal),
        diff_direction,
        material.roughness * material.roughness * (1.0 - is_specular)
    ));
    ray.origin = position;
}