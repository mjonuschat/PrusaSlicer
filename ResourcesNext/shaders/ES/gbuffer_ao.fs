#version 100

uniform vec4 uniform_color;
uniform int material_id;

varying vec4 light_position;
varying vec3 eye_normal;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

void main()
{
    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = uniform_color;
}