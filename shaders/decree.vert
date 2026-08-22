#version 330 core

layout(location = 0) in vec2  ring_pos;   // unit circle point
layout(location = 1) in vec2  inst_pos;
layout(location = 2) in float inst_radius;
layout(location = 3) in float inst_t;     // lifetime [0,1]
layout(location = 4) in vec3  inst_color;

uniform mat4 view_proj;

out vec3  frag_color;
out float frag_t;

void main() {
    vec2 world_pos = inst_pos + ring_pos * inst_radius;
    gl_Position = view_proj * vec4(world_pos, 0.0, 1.0);
    frag_color = inst_color;
    frag_t     = inst_t;
}
