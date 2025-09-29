#version 330

uniform vec4 uniform_color;
uniform int material_id;
uniform float out_of_bed_threshold_z;

in vec3 eye_normal;
in vec4 light_position;
in float world_z;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

vec4 select_color()
{
    return (world_z >= out_of_bed_threshold_z) ? uniform_color : vec4(mix(uniform_color.rgb, vec3(0.0), 0.333), uniform_color.a);
}

void main()
{
    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = select_color();
}