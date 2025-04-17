#version 330

uniform vec4 uniform_color;

in vec3 eye_position;
in vec4 light_position;
in vec3 eye_normal;

layout (location = 0) out vec3 g_eye_position;
layout (location = 1) out vec4 g_light_position;
layout (location = 2) out vec3 g_eye_normal;
layout (location = 3) out vec4 g_color;

void main()
{
    g_eye_normal = normalize(eye_normal);
    g_eye_position = eye_position;
    g_light_position = light_position;
    g_color = uniform_color;
}