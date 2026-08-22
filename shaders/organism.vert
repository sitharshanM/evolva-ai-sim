#version 330 core

// ── Per-vertex (triangle template, unit size) ──────────────────────────────
layout(location = 0) in vec2 vertex_pos;

// ── Per-instance ──────────────────────────────────────────────────────────
layout(location = 1) in vec2  inst_pos;
layout(location = 2) in float inst_angle;
layout(location = 3) in float inst_radius;
layout(location = 4) in vec3  inst_color;
layout(location = 5) in float inst_energy;
layout(location = 6) in float inst_selected;

uniform mat4 view_proj;

out vec3  frag_color;
out float frag_energy;
out float frag_selected;
out vec2  frag_local;  // local position (for outline effect)

void main() {
    float c = cos(inst_angle);
    float s = sin(inst_angle);

    // Rotate template around local origin then scale
    vec2 rotated = vec2(
        vertex_pos.x * c - vertex_pos.y * s,
        vertex_pos.x * s + vertex_pos.y * c
    ) * inst_radius;

    vec2 world_pos = inst_pos + rotated;
    gl_Position = view_proj * vec4(world_pos, 0.0, 1.0);

    frag_color    = inst_color;
    frag_energy   = inst_energy;
    frag_selected = inst_selected;
    frag_local    = vertex_pos;
}
