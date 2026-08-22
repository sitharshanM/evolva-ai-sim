#version 330 core
layout(location = 0) in vec2 pos;
uniform mat4 view_proj;
void main() {
    gl_Position = view_proj * vec4(pos, 0.0, 1.0);
}
