#version 330 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D crisp_notes;
uniform sampler2D bloom_blur;
uniform float bloom_intensity;

void main()
{
    vec3 base_color = vec3(0.1, 0.1, 0.1);
    vec3 notes = texture(crisp_notes, tex_coords).rgb;
    vec3 glow = texture(bloom_blur, tex_coords).rgb;

    vec3 final_color = base_color + notes + (glow * bloom_intensity);

    final_color *= 0.6;

    frag_color = vec4(final_color, 1.0);
}
