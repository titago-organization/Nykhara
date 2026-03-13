#version 450

// SDF Text — Vertex Shader
// Renders a glyph quad from the font atlas.

layout(push_constant) uniform PushConstants {
    vec4 transform;   // x_offset, y_offset, scale_x, scale_y
    vec4 rect;        // glyph quad: x, y, w, h (pixels)
    vec4 color;       // text color rgba
    vec4 uv_rect;     // atlas UV: u0, v0, u1, v1
    vec4 params;      // sdf_threshold, sdf_softness, _, _
} pc;

layout(location = 0) out vec2 v_uv;

void main() {
    vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );

    vec2 pos = positions[gl_VertexIndex];

    // UV from atlas
    v_uv = mix(pc.uv_rect.xy, pc.uv_rect.zw, pos);

    // Position
    vec2 pixel = pc.rect.xy + pos * pc.rect.zw;
    gl_Position = vec4(
        pc.transform.x + pixel.x * pc.transform.z,
        pc.transform.y + pixel.y * pc.transform.w,
        0.0, 1.0
    );
}

