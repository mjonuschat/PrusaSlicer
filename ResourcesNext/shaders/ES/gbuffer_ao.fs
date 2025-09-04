#version 100

const float THRESHOLD_Z = -0.15;

uniform vec4 uniform_color;
uniform int material_id;
uniform bool enable_out_of_bed_detection_z;

varying vec3 eye_normal;
varying vec4 light_position;
varying float world_z;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

vec4 select_color()
{
    return (!enable_out_of_bed_detection_z || world_z >= THRESHOLD_Z) ? uniform_color : vec4(mix(uniform_color.rgb, vec3(0.0), 0.333), uniform_color.a);
}

void main()
{
    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = select_color();
}