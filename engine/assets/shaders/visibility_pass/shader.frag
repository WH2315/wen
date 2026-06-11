#version 450

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in flat uint instance_id;

layout(location = 0) out uvec2 out_visibility;

layout(early_fragment_tests) in;
void main() {
    out_visibility = uvec2(instance_id, gl_PrimitiveID + 1);
}
