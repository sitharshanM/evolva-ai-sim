#version 330 core

layout(location = 0) in vec2 vertex_pos;   // quad [-1,1]
layout(location = 1) in vec2  inst_pos;
layout(location = 2) in float inst_radius;
layout(location = 3) in float inst_alpha;

uniform mat4 view_proj;

out vec2  frag_uv;
out float frag_alpha;

void main() {
    vec2 world_pos = inst_pos + vertex_pos * inst_radius;
    gl_Position = view_proj * vec4(world_pos, 0.0, 1.0);
    frag_uv     = vertex_pos;   // [-1,1]
    frag_alpha  = inst_alpha;
}
