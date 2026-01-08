#version 100

precision highp float;

uniform sampler2D texture;
uniform bool transparent_background;
uniform vec3 back_color_dark;
uniform vec3 back_color_light;

varying vec2 tex_coord;

vec4 gradient_color()
{
    // takes foreground from texture
    vec4 fore_color = texture2D(texture, tex_coord);

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

    gl_FragColor = color;
}