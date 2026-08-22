#version 330 core

in vec3  frag_color;
in float frag_t;  // 1.0 = just appeared, 0.0 = fading out

out vec4 out_color;

void main() {
    // Fade in quickly, then slowly fade out
    float alpha = frag_t * frag_t * 0.85;
    // Pulse effect
    float pulse = 0.7 + 0.3 * sin(frag_t * 12.0);
    out_color = vec4(frag_color * pulse, alpha);
}
