#version 330

uniform sampler2D in_texture;
uniform int material_id;
uniform bool transparent_background;
uniform vec3 back_color_dark;
uniform vec3 back_color_light;

in vec4 light_position;
in vec3 eye_normal;
in vec2 tex_coord;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

vec4 gradient_color()
{
    // takes foreground from texture
    vec4 fore_color = texture(in_texture, tex_coord);

    // calculates radial gradient
    vec3 back_color = vec3(mix(back_color_light, back_color_dark, smoothstep(0.0, 0.5, length(abs(tex_coord.xy) - vec2(0.5)))));

    // blends foreground with background
    return vec4(mix(back_color, fore_color.rgb, fore_color.a), transparent_background ? fore_color.a : 1.0);
}

void main()
{
    vec4 color = gradient_color();
    color.a = transparent_background ? color.a * 0.5 : color.a;

    if (color.a == 0.0)
        discard;

    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = color;
}