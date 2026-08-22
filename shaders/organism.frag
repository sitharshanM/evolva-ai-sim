#version 330 core

in vec3  frag_color;
in float frag_energy;
in float frag_selected;
in vec2  frag_local;

out vec4 out_color;

void main() {
    // Brightness scaled by energy: low energy = dark, purple tint
    float brightness = 0.25 + 0.75 * clamp(frag_energy, 0.0, 1.0);
    vec3 col = frag_color * brightness;

    // Low-energy organisms get a reddish-purple tint (warning)
    if (frag_energy < 0.3) {
        float t = 1.0 - frag_energy / 0.3;
        col = mix(col, vec3(0.8, 0.1, 0.4), t * 0.6);
    }

    // Selected organism: white glow outline
    float alpha = 0.90;
    if (frag_selected > 0.5) {
        // Distance from edge of triangle → glow effect
        float edge = min(1.0 - abs(frag_local.x), 1.0 - abs(frag_local.y));
        float glow = smoothstep(0.0, 0.25, edge);
        col = mix(vec3(1.0, 1.0, 1.0), col, glow);
        alpha = 1.0;
    }

    out_color = vec4(col, alpha);
}
