#version 330

const vec3 back_color_dark  = vec3(0.235, 0.235, 0.235);
const vec3 back_color_light = vec3(0.365, 0.365, 0.365);

uniform sampler2D in_texture;
uniform bool transparent_background;
uniform bool svg_source;

in vec3 eye_position;
in vec4 light_position;
in vec3 eye_normal;
in vec2 tex_coord;

layout (location = 0) out vec3 g_eye_position;
layout (location = 1) out vec4 g_light_position;
layout (location = 2) out vec3 g_eye_normal;
layout (location = 3) out vec4 g_color;

vec4 svg_color()
{
    // takes foreground from texture
    vec4 fore_color = texture(in_texture, tex_coord);

    // calculates radial gradient
    vec3 back_color = vec3(mix(back_color_light, back_color_dark, smoothstep(0.0, 0.5, length(abs(tex_coord.xy) - vec2(0.5)))));

    // blends foreground with background
    return vec4(mix(back_color, fore_color.rgb, fore_color.a), transparent_background ? fore_color.a : 1.0);
}

vec4 non_svg_color()
{
    // takes foreground from texture
    vec4 color = texture(in_texture, tex_coord);
    return vec4(color.rgb, transparent_background ? color.a * 0.25 : color.a);
}

void main()
{
    vec4 color = svg_source ? svg_color() : non_svg_color();
    color.a = transparent_background ? color.a * 0.5 : color.a;

    g_eye_normal = normalize(eye_normal);
    g_eye_position = eye_position;
    g_light_position = light_position;
    g_color = color;
}