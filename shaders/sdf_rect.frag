#version 450

// SDF Shape Morph — Fragment Shader
//
// Two modes:
//   params.z == 0 → Rounded rect mode
//   params.z > 0  → Shape morph: 0→1 (tri→sq), 1→2 (sq→circle)
//
// Morph blends SDF distance fields with mix(), giving pixel-perfect
// smooth contour transitions.

layout(push_constant) uniform PushConstants {
    vec4 transform;
    vec4 rect;
    vec4 color;
    vec4 corners;
    vec4 params;   // border_width, softness, morph_t, corner_round
} pc;

layout(location = 0) in vec2 v_local;
layout(location = 0) out vec4 out_color;

// ─── SDF primitives ───

float sdf_rounded_rect(vec2 p, vec2 half_size, vec4 radii) {
    float r = (p.x > 0.0)
        ? ((p.y > 0.0) ? radii.z : radii.y)
        : ((p.y > 0.0) ? radii.w : radii.x);
    vec2 q = abs(p) - half_size + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

// Regular N-gon SDF — precompute angle and radius once
float sdf_ngon(vec2 p, float radius, float n, float angle, float len) {
    float an = 3.14159265 / n;
    float he = radius * cos(an);
    float a = mod(angle + an, 2.0 * an) - an;
    return len * cos(a) - he;
}

float sdf_morph(vec2 p, float radius, float morph_t, float corner_r) {
    float inner_r = radius - corner_r;

    // Precompute shared values once
    float angle = atan(p.x, p.y);
    float len = length(p);

    // Only compute the two SDFs we actually need
    float d;
    if (morph_t <= 1.0) {
        float d_a = sdf_ngon(p, inner_r, 3.0, angle, len);
        float d_b = sdf_ngon(p, inner_r, 4.0, angle, len);
        d = mix(d_a, d_b, morph_t);
    } else {
        float d_a = sdf_ngon(p, inner_r, 4.0, angle, len);
        float d_b = len - inner_r;  // circle SDF inlined
        d = mix(d_a, d_b, morph_t - 1.0);
    }

    return d - corner_r;
}

void main() {
    vec2 half_size = pc.rect.zw * 0.5;
    vec2 p = v_local - half_size;

    float morph_t = pc.params.z;
    float dist;

    if (morph_t > 0.001) {
        float radius = min(half_size.x, half_size.y);
        dist = sdf_morph(p, radius, morph_t, pc.params.w);
    } else {
        dist = sdf_rounded_rect(p, half_size, pc.corners);
    }

    float softness = max(pc.params.y, 0.6);
    float alpha = 1.0 - smoothstep(-softness, softness, dist);

    float border = pc.params.x;
    if (border > 0.0) {
        float inner_dist;
        if (morph_t > 0.001) {
            float radius = min(half_size.x, half_size.y);
            inner_dist = sdf_morph(p, radius - border, morph_t, max(pc.params.w - border, 0.0));
        } else {
            inner_dist = sdf_rounded_rect(p, half_size - border,
                                           max(pc.corners - border, vec4(0.0)));
        }
        alpha *= smoothstep(-softness, softness, inner_dist);
    }

    out_color = vec4(pc.color.rgb * pc.color.a * alpha, pc.color.a * alpha);
}
