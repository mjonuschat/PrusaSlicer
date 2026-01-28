#version 300 es

precision highp float;

uniform int material_id;

in vec3 eye_normal;
in vec4 color;

layout (location = 0) out vec4 g_eye_normal;
layout (location = 1) out vec4 g_color;

void main()
{
    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_color = color;
}
