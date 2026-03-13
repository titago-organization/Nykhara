#version 450

// SDF Rounded Rect — Vertex Shader
// Generates a fullscreen quad from vertex ID (6 verts, 2 triangles).

layout(push_constant) uniform PushConstants {
    vec4 transform;   // x_offset, y_offset, scale_x, scale_y
    vec4 rect;        // x, y, w, h (pixel coords)
    vec4 color;       // rgba
    vec4 corners;     // corner radii: TL, TR, BR, BL
    vec4 params;      // border_width, softness, _, _
} pc;

layout(location = 0) out vec2 v_local;  // Local position in rect space [0..w, 0..h]

void main() {
    // 6 vertices → 2 triangles covering the rect
    vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );

    vec2 pos = positions[gl_VertexIndex];

    // Expand slightly for antialiasing
    float aa_expand = 2.0;
    vec2 size = pc.rect.zw + aa_expand * 2.0;
    vec2 origin = pc.rect.xy - aa_expand;

    v_local = pos * size - vec2(aa_expand);

    // To NDC
    vec2 pixel = origin + pos * size;
    gl_Position = vec4(
        pc.transform.x + pixel.x * pc.transform.z,
        pc.transform.y + pixel.y * pc.transform.w,
        0.0, 1.0
    );
}

