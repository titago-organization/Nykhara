#version 450

// SDF Text — Fragment Shader
// Samples the SDF font atlas and produces crisp text at any scale.

layout(push_constant) uniform PushConstants {
    vec4 transform;
    vec4 rect;
    vec4 color;
    vec4 uv_rect;
    vec4 params;      // sdf_threshold, sdf_softness, _, _
} pc;

layout(set = 0, binding = 0) uniform sampler2D font_atlas;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main() {
    float dist = texture(font_atlas, v_uv).r;

    float threshold = pc.params.x;  // ~0.5 (128/255 for stb_truetype SDF)
    float softness  = pc.params.y;  // ~0.1 for screen-space AA

    // Screen-space derivative for scale-independent AA
    float dx = dFdx(dist);
    float dy = dFdy(dist);
    float aa = length(vec2(dx, dy)) * 0.7;
    float effective_softness = max(softness, aa);

    float alpha = smoothstep(threshold - effective_softness,
                             threshold + effective_softness, dist);

    out_color = vec4(pc.color.rgb, pc.color.a * alpha);
    if (out_color.a < 0.001) discard;
}

