#version 330 core

in vec2  frag_uv;
in float frag_alpha;

out vec4 out_color;

void main() {
    // Soft circle from quad: discard outside radius
    float d = length(frag_uv);
    if (d > 1.0) discard;

    // Bright green core with glow falloff
    float core  = 1.0 - smoothstep(0.0, 0.5, d);
    float glow  = 1.0 - smoothstep(0.4, 1.0, d);

    vec3 inner  = vec3(0.35, 1.0, 0.4);   // bright lime green
    vec3 outer  = vec3(0.15, 0.65, 0.25); // darker green

    vec3 color  = mix(outer, inner, core);
    float alpha = glow * frag_alpha;

    out_color = vec4(color, alpha);
}
