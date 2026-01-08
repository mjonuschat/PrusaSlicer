#version 330

uniform vec4 uniform_color;
uniform int material_id;
// Clipping planes, x = min z, y = max z. Used by the SLA preview to clip with a top / bottom plane.
uniform vec2 z_range;

in vec4 light_position;
in vec3 eye_normal;
in float world_z;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

void main()
{
    if (world_z < z_range.x || z_range.y < world_z)
        discard;

    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = uniform_color;
}