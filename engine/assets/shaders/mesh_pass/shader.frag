#version 450

#extension GL_GOOGLE_include_directive : require

#include "../utils.glsl"
#include "../environment/common.glsl"

struct VkDrawIndexedIndirectCommand {
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

layout(binding = 0, input_attachment_index = 0) uniform usubpassInput visibility_buffer;

layout(binding = 1) uniform Camera {
    mat4 view;
    mat4 project;
} camera;

layout(std430, binding = 2) readonly buffer IndexBuffer {
    uint indices[];
};

layout(std430, binding = 3) readonly buffer PositionBuffer {
    float positions[];
};

layout(std430, binding = 4) readonly buffer NormalBuffer {
    float normals[];
};

layout(std430, binding = 5) readonly buffer TexcoordBuffer {
    float texcoords[];
};

layout(std430, binding = 6) readonly buffer ColorBuffer {
    float colors[];
};

layout(std430, binding = 17) readonly buffer TangentBuffer {
    float tangents[];
};

layout(std430, binding = 7) readonly buffer InstanceData {
    vec4 instance_datas[];
};

layout(std430, binding = 8) readonly buffer Counts {
    uint visible_mesh_instance_count;
    uint available_indirect_command_count;
};

layout(std430, binding = 9) readonly buffer AvailableIndirectCommands {
    VkDrawIndexedIndirectCommand available_indirect_commands[];
};

const uint MAX_TEXTURE_COUNT = 64;
layout(binding = 10) uniform sampler2D albedo_textures[MAX_TEXTURE_COUNT];
layout(binding = 16) uniform sampler2D normal_textures[MAX_TEXTURE_COUNT];
layout(binding = 18) uniform sampler2D mr_textures[MAX_TEXTURE_COUNT];
layout(binding = 19) uniform sampler2D ao_textures[MAX_TEXTURE_COUNT];

struct Light {
    vec4 position_type;    // .xyz = 位置(点/聚光), .w = 类型 0=方向 1=点 2=聚光
    vec4 color_intensity;  // .xyz = 颜色, .w = 强度
    vec4 range_angle;      // .x = 衰减范围, .y = 内锥角cos, .z = 外锥角cos
    vec4 direction;        // 照射方向
};
const uint MAX_LIGHT_COUNT = 16;
layout(std430, binding = 11) readonly buffer LightsBuffer {
    uint light_count;
    Light lights[MAX_LIGHT_COUNT];
};

// IBL 环境资源(EnvironmentSystem 预积分生成)
layout(binding = 12) uniform samplerCube env_map;
layout(binding = 13) uniform samplerCube irradiance_map;
layout(binding = 14) uniform samplerCube prefiltered_map;
layout(binding = 15) uniform sampler2D brdf_lut;

// 预过滤立方体贴图的 mip 层数(决定 textureLod 的最大层)。
const float PREFILTER_MIP_LEVELS = 5.0;

layout(location = 0) in vec2 ndc_pos;
layout(location = 0) out vec4 out_color;

// LOD 调试开关(push constant,Setting 面板勾选):>0.5 时按当前选中的 LOD 层级着色。
layout(push_constant) uniform Constants {
    float lod_debug_enabled;
} pc;

vec2 interpolate_vec2(mat3x2 attributes, vec3 db_dx, vec3 db_dy, vec2 delta) {
	vec3 attr0 = vec3(attributes[0].x, attributes[1].x, attributes[2].x);
	vec3 attr1 = vec3(attributes[0].y, attributes[1].y, attributes[2].y);
	vec2 attribute_x = vec2(dot(db_dx,attr0), dot(db_dx,attr1));
	vec2 attribute_y = vec2(dot(db_dy,attr0), dot(db_dy,attr1));
	vec2 attribute_s = attributes[0];
	vec2 result = (attribute_s + delta.x * attribute_x + delta.y * attribute_y);
	return result;
}

vec3 interpolate_vec3(mat3 attributes, vec3 db_dx, vec3 db_dy, vec2 delta) {
	vec3 attribute_x = attributes * db_dx;
	vec3 attribute_y = attributes * db_dy;
	vec3 attribute_s = attributes[0];
	return (attribute_s + delta.x * attribute_x + delta.y * attribute_y);
}

float interpolate_float(vec3 attributes, vec3 db_dx, vec3 db_dy, vec2 delta) {
    float attribute_x = dot(attributes, db_dx);
    float attribute_y = dot(attributes, db_dy);
    float attribute_s = attributes[0];
    return (attribute_s + delta.x * attribute_x + delta.y * attribute_y);
}

// 轻量 ACES 电影级色调映射(IBL 环境光可能超过 1.0,HDR -> LDR)。
vec3 aces_film(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec4 shading(vec3 pos, vec3 normal, vec2 tex_coord, vec3 color, vec3 base_color, uint texture_index, float metallic, float roughness, uint lod_index, vec3 emissive, float occlusion) {
    vec3 albedo = base_color * color;
    if (texture_index > 0u && texture_index < MAX_TEXTURE_COUNT) {
        albedo *= texture(albedo_textures[texture_index], tex_coord).rgb;
    }

    // LOD 调试:按当前选中的 LOD 层级着色(与旧版 asset_system 烘焙色一致,改为渲染期计算)。
    if (pc.lod_debug_enabled > 0.5) {
        float c = pow(float(lod_index) / 7.0, 0.8);
        albedo = vec3(c, 0.7 - abs(0.5 - c), 1.0 - c);
    }

    // 相机位置(取视图矩阵逆的第 4 列)用于高光视线方向
    vec3 cam_pos = (inverse(camera.view) * vec4(0, 0, 0, 1)).xyz;
    vec3 view_dir = normalize(cam_pos - pos);
    float ndv = clamp(dot(normal, view_dir), 0.0, 1.0);

    // IBL 环境光:漫反射辐照度 + 预过滤镜面 + BRDF LUT
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 irradiance = texture(irradiance_map, normal).rgb;
    vec3 diffuse_ibl = irradiance * albedo;
    vec3 r = reflect(-view_dir, normal);
    vec3 prefiltered = textureLod(prefiltered_map, r, roughness * (PREFILTER_MIP_LEVELS - 1.0)).rgb;
    vec2 brdf = texture(brdf_lut, vec2(ndv, roughness)).rg;
    vec3 fresnel_ibl = fresnel_schlick_roughness(ndv, f0, roughness);
    vec3 specular_ibl = prefiltered * (fresnel_ibl * brdf.x + brdf.y);

    // 金属度抑制漫反射(金属几乎没有漫反射)
    vec3 kd_ibl = mix(vec3(1.0), vec3(0.0), metallic);
    vec3 lighting = kd_ibl * diffuse_ibl + specular_ibl;

    // 逐点光源:Cook-Torrance 镜面 BRDF(GGX 分布 + Smith 几何 + Schlick 菲涅尔) + 漫反射。
    for (uint i = 0u; i < min(light_count, MAX_LIGHT_COUNT); i++) {
        Light light = lights[i];
        vec3 light_color = light.color_intensity.rgb * light.color_intensity.w;
        vec3 light_dir;
        float attenuation = 1.0;

        if (light.position_type.w < 0.5) {
            // 方向光
            light_dir = normalize(-light.direction.xyz);
        } else {
            // 点光 / 聚光:方向朝光源,距离平方衰减
            vec3 to_light = light.position_type.xyz - pos;
            float dist = length(to_light);
            light_dir = to_light / max(dist, 1e-4);
            float range = max(light.range_angle.x, 1e-3);
            attenuation = 1.0 / (1.0 + (dist * dist) / (range * range));
            if (light.position_type.w >= 1.5) {
                // 聚光锥角衰减:smoothstep(外角cos, 内角cos, 当前cos)
                float cos_angle = dot(light_dir, normalize(light.direction.xyz));
                attenuation *= smoothstep(light.range_angle.z, light.range_angle.y, cos_angle);
            }
        }

        vec3 half_dir = normalize(light_dir + view_dir);
        float ndl = max(dot(normal, light_dir), 0.0);
        if (ndl <= 0.0) {
            continue;
        }

        // 镜面 Cook-Torrance: D * G * F / (4 * NdotV * NdotL)
        float ndf = distribution_ggx(normal, half_dir, roughness);
        float g = geometry_smith(normal, view_dir, light_dir, roughness);
        vec3 f = fresnel_schlick(max(dot(half_dir, view_dir), 0.0), f0);
        float denom = 4.0 * max(ndv, 1e-4) * ndl;
        vec3 specular = ndf * g * f / max(denom, 1e-4);

        // 漫反射系数:Fresnel 已折走的能量 * (1 - metallic)
        vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);
        lighting += (kd * albedo / PI + specular) * light_color * ndl * attenuation;
    }
    return vec4(lighting * occlusion + emissive, 1);
}

vec4 compute_out_color(VkDrawIndexedIndirectCommand command, uint instance_id, uint triangle_id) {
    // 获取索引偏移
    uint offset = command.first_index + triangle_id * 3;

    // 获取索引
    uint idx0 = (command.vertex_offset + indices[offset + 0]) * 3;
    uint idx1 = (command.vertex_offset + indices[offset + 1]) * 3;
    uint idx2 = (command.vertex_offset + indices[offset + 2]) * 3;
    uint idx_uv0 = (command.vertex_offset + indices[offset + 0]) * 2;
    uint idx_uv1 = (command.vertex_offset + indices[offset + 1]) * 2;
    uint idx_uv2 = (command.vertex_offset + indices[offset + 2]) * 2;

    // 获取三角形三个顶点的属性
    vec3 position0 = vec3(positions[idx0 + 0], positions[idx0 + 1], positions[idx0 + 2]);
    vec3 position1 = vec3(positions[idx1 + 0], positions[idx1 + 1], positions[idx1 + 2]);
    vec3 position2 = vec3(positions[idx2 + 0], positions[idx2 + 1], positions[idx2 + 2]);
    vec3 normal0 = vec3(normals[idx0 + 0], normals[idx0 + 1], normals[idx0 + 2]);
    vec3 normal1 = vec3(normals[idx1 + 0], normals[idx1 + 1], normals[idx1 + 2]);
    vec3 normal2 = vec3(normals[idx2 + 0], normals[idx2 + 1], normals[idx2 + 2]);
    vec2 tex_coord0 = vec2(texcoords[idx_uv0 + 0], texcoords[idx_uv0 + 1]);
    vec2 tex_coord1 = vec2(texcoords[idx_uv1 + 0], texcoords[idx_uv1 + 1]);
    vec2 tex_coord2 = vec2(texcoords[idx_uv2 + 0], texcoords[idx_uv2 + 1]);
    vec3 color0 = vec3(colors[idx0 + 0], colors[idx0 + 1], colors[idx0 + 2]);
    vec3 color1 = vec3(colors[idx1 + 0], colors[idx1 + 1], colors[idx1 + 2]);
    vec3 color2 = vec3(colors[idx2 + 0], colors[idx2 + 1], colors[idx2 + 2]);
    vec3 tangent0 = vec3(tangents[idx0 + 0], tangents[idx0 + 1], tangents[idx0 + 2]);
    vec3 tangent1 = vec3(tangents[idx1 + 0], tangents[idx1 + 1], tangents[idx1 + 2]);
    vec3 tangent2 = vec3(tangents[idx2 + 0], tangents[idx2 + 1], tangents[idx2 + 2]);

    // 获取实例的变换矩阵和相机的变换矩阵
    mat3 model = compute_model(instance_datas[instance_id * 9 + 2].xyz, instance_datas[instance_id * 9 + 1].xyz);
    mat4 project_view = camera.project * camera.view;

    // 计算世界坐标系下的三角形顶点位置
    vec3 world_pos0 = instance_datas[instance_id * 9 + 0].xyz + model * position0;
    vec3 world_pos1 = instance_datas[instance_id * 9 + 0].xyz + model * position1;
    vec3 world_pos2 = instance_datas[instance_id * 9 + 0].xyz + model * position2;

    // 计算NDC坐标系下的三角形顶点位置
    vec4 ndc_pos0 = project_view * vec4(world_pos0, 1);
    vec4 ndc_pos1 = project_view * vec4(world_pos1, 1);
    vec4 ndc_pos2 = project_view * vec4(world_pos2, 1);

    // 除以w, 归一化
    vec3 one_div_w = 1.0 / vec3(ndc_pos0.w, ndc_pos1.w, ndc_pos2.w);
    ndc_pos0 *= one_div_w.x;
    ndc_pos1 *= one_div_w.y;
    ndc_pos2 *= one_div_w.z;

    // 根据当前像素和三角形顶点位置，计算偏导，用于插值顶点属性
    // 因为在ndc空间计算的偏导，所以插值不用考虑透视矫正
    vec2 v[3] = vec2[](
        ndc_pos0.xy,
        ndc_pos1.xy,
        ndc_pos2.xy
    );
    float d = 1.0 / determinant(mat2(v[2] - v[1], v[0] - v[1]));
	vec3 db_dx = vec3(v[1].y - v[2].y, v[2].y - v[0].y, v[0].y - v[1].y) * d;
	vec3 db_dy = vec3(v[2].x - v[1].x, v[0].x - v[2].x, v[1].x - v[0].x) * d;
    vec2 delta = ndc_pos - v[0];

    // 对所有顶点属性插值
    vec3 pos = interpolate_vec3(mat3(world_pos0, world_pos1, world_pos2), db_dx, db_dy, delta);
    vec3 normal = normalize(interpolate_vec3(mat3(normal0, normal1, normal2), db_dx, db_dy, delta));
    vec2 uv = interpolate_vec2(mat3x2(tex_coord0, tex_coord1, tex_coord2), db_dx, db_dy, delta);
    vec3 color = interpolate_vec3(mat3(color0, color1, color2), db_dx, db_dy, delta);
    // 因为model矩阵可能包含旋转和缩放，所以需要重新计算法向量
    normal = normalize(transpose(inverse(model)) * normal);

    // 切线 -> 世界,并相对法线正交化,构成 TBN。
    vec3 tangent = normalize(interpolate_vec3(mat3(tangent0, tangent1, tangent2), db_dx, db_dy, delta));
    tangent = normalize(mat3(model) * tangent);
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = cross(normal, tangent);

    // 材质数据:第 4 个 vec4 = (base_color.rgb, texture_index);第 5 个 vec4 = (metallic, roughness, 0, lod_index)
    vec4 material_data = instance_datas[instance_id * 9 + 3];
    vec3 base_color = material_data.rgb;
    uint texture_index = uint(material_data.w + 0.5);
    vec4 pbr_data = instance_datas[instance_id * 9 + 4];
    float metallic = clamp(pbr_data.x, 0.0, 1.0);
    float roughness = clamp(pbr_data.y, 0.04, 1.0);
    uint lod_index = uint(pbr_data.w + 0.5);  // compact_instance.comp 写入选中的 LOD 层级
    // 第 6 个 vec4 = (emissive.rgb, intensity)
    vec4 emissive_data = instance_datas[instance_id * 9 + 5];
    vec3 emissive = emissive_data.rgb * emissive_data.w;
    // 第 7 个 vec4 = UV 平铺 (tiling.x, tiling.y)
    vec2 tiling = instance_datas[instance_id * 9 + 6].xy;
    uv *= tiling;

    // 第 8 个 vec4 = 法线贴图 (index, scale)
    vec4 normal_data = instance_datas[instance_id * 9 + 7];
    uint normal_index = uint(normal_data.x + 0.5);
    float normal_scale = normal_data.y;

    // 第 9 个 vec4 = MR/AO (mr_index, ao_index, ao_intensity)
    vec4 mr_ao_data = instance_datas[instance_id * 9 + 8];
    uint mr_index = uint(mr_ao_data.x + 0.5);
    uint ao_index = uint(mr_ao_data.y + 0.5);
    float ao_intensity = mr_ao_data.z;

    // metallic-roughness 贴图调制(B=金属度, G=粗糙度)。
    if (mr_index > 0u && mr_index < MAX_TEXTURE_COUNT) {
        vec3 mr = texture(mr_textures[mr_index], uv).rgb;
        metallic = clamp(metallic * mr.b, 0.0, 1.0);
        roughness = clamp(roughness * mr.g, 0.04, 1.0);
    }

    // AO:环境光遮蔽乘到最终光照。
    float occlusion = 1.0;
    if (ao_index > 0u && ao_index < MAX_TEXTURE_COUNT) {
        float ao = max(texture(ao_textures[ao_index], uv).r, 0.0);
        occlusion = mix(1.0, ao, clamp(ao_intensity, 0.0, 1.0));
    }

    // 法线贴图扰动:经 TBN 把切线空间法线转到世界空间(无法线贴图时用几何法线)。
    vec3 shade_normal = normal;
    if (normal_index > 0u && normal_index < MAX_TEXTURE_COUNT) {
        vec3 n_raw = texture(normal_textures[normal_index], uv).rgb * 2.0 - 1.0;
        n_raw.xy *= normal_scale;
        shade_normal = normalize(tangent * n_raw.x + bitangent * n_raw.y + normal * n_raw.z);
    }

    // 进行着色
    return shading(pos, shade_normal, uv, color, base_color, texture_index, metallic, roughness, lod_index, emissive, occlusion);
}

void main() {
    uvec2 primitive_info = subpassLoad(visibility_buffer).xy;
    if (primitive_info.x + primitive_info.y == 0) {
        // 天空盒背景:由 NDC 反投影得到世界方向,采样环境立方体贴图
        vec4 view_ray = inverse(camera.project) * vec4(ndc_pos, 1.0, 1.0);
        vec3 view_dir = view_ray.xyz / view_ray.w;
        vec3 world_dir = normalize((inverse(camera.view) * vec4(view_dir, 0.0)).xyz);
        vec3 sky_color = texture(env_map, world_dir).rgb;
        out_color = vec4(aces_film(sky_color), 1.0);
        return;
    }

    for (int i = 0; i < available_indirect_command_count; i++) {
        VkDrawIndexedIndirectCommand command = available_indirect_commands[i];
        if(step(command.first_instance, primitive_info.x) * step(primitive_info.x, command.first_instance + command.instance_count - 1) == 1) {
            vec3 hdr_color = compute_out_color(command, primitive_info.x, primitive_info.y - 1).rgb;
            out_color = vec4(aces_film(hdr_color), 1.0);
            return;
        }
    }
    out_color = vec4(0);
}
