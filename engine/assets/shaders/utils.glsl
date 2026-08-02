
mat3 compute_model(vec3 scale, vec3 rotation) {
    rotation *= 0.01745329252;  // degrees to radians
    float sin_v = sin(rotation.x), cos_v = cos(rotation.x);
    mat3 rotate_x = mat3(
        1, 0, 0,
        0, cos_v, sin_v,
        0, -sin_v, cos_v
    );
    sin_v = sin(rotation.y), cos_v = cos(rotation.y);
    mat3 rotate_y = mat3(
        cos_v, 0, -sin_v,
        0, 1, 0,
        sin_v, 0, cos_v
    );
    sin_v = sin(rotation.z), cos_v = cos(rotation.z);
    mat3 rotate_z = mat3(
        cos_v, sin_v, 0,
        -sin_v, cos_v, 0,
        0, 0, 1
    );
    mat3 scale_m = mat3(scale.x, 0, 0, 0, scale.y, 0, 0, 0, scale.z);
    // Standard TRS: scale in local space first, then rotate (X, then Y, then Z).
    // Matches ImGuizmo's decomposition so the gizmo and the render agree.
    return rotate_z * rotate_y * rotate_x * scale_m;
}