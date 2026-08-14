// IBL 常用工具:立方体方向映射 / 蒙特卡洛采样
// (equirect_to_cubemap / irradiance / prefilter / brdf_lut / mesh shader 共享)

#ifndef WEN_IBL_COMMON_GLSL
#define WEN_IBL_COMMON_GLSL

const float PI = 3.141592653589793;

// 立方体面 UV -> 方向。Vulkan 面序:+X,-X,+Y,-Y,+Z,-Z。
// 图像坐标 y 向下,先翻转再映射,保证生成与采样一致。
vec3 cube_dir(int face, vec2 uv) {
    uv.y = 1.0 - uv.y;
    vec2 c = uv * 2.0 - 1.0;
    switch (face) {
        case 0:  return vec3( 1.0,  c.y, -c.x); // +X
        case 1:  return vec3(-1.0,  c.y,  c.x); // -X
        case 2:  return vec3( c.x,  1.0, -c.y); // +Y
        case 3:  return vec3( c.x, -1.0,  c.y); // -Y
        case 4:  return vec3( c.x,  c.y,  1.0); // +Z
        default: return vec3(-c.x,  c.y, -1.0); // -Z
    }
}

// ---------- 蒙特卡洛采样 ----------

float radical_inverse_vdc(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n) {
    return vec2(float(i) / float(n), radical_inverse_vdc(i));
}

// GGX 重要性采样:给定随机数 (xi) 与法线 n,返回半程向量方向。
vec3 importance_sample_ggx(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    vec3 h = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

float geometry_schlick_ggx(float ndotv, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    return ndotv / (ndotv * (1.0 - k) + k);
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness) {
    float ndotv = max(dot(n, v), 0.0);
    float ndotl = max(dot(n, l), 0.0);
    return geometry_schlick_ggx(ndotv, roughness) * geometry_schlick_ggx(ndotl, roughness);
}

// BRDF LUT 积分(分解后的 Cook-Torrance 镜面项,见 LearnOpenGL "IBL/Specular IBL")。
// Schlick 菲涅尔(带粗糙度近似,用于环境光镜面混合)。
vec3 fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec2 integrate_brdf(float ndotv, float roughness) {
    vec3 v = vec3(sqrt(1.0 - ndotv * ndotv), 0.0, ndotv);
    vec3 n = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024u;
    float a = 0.0, b = 0.0;
    for (uint i = 0u; i < SAMPLE_COUNT; i++) {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 h = importance_sample_ggx(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);
        float ndotl = max(l.z, 0.0);
        if (ndotl > 0.0) {
            float ndoth = max(h.z, 0.0);
            float vdoth = max(dot(v, h), 0.0);
            float g = geometry_smith(n, v, l, roughness);
            float g_vis = (g * vdoth) / (ndoth * ndotv);
            float fc = pow(1.0 - vdoth, 5.0);
            a += (1.0 - fc) * g_vis;
            b += fc * g_vis;
        }
    }
    return vec2(a, b) / float(SAMPLE_COUNT);
}

#endif  // WEN_IBL_COMMON_GLSL
