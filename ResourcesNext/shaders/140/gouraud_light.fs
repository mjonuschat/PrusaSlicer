#version 140

uniform vec4 uniform_color;
uniform float out_of_bed_threshold_z;

// x = tainted, y = specular;
in vec2 intensity;
in float world_z;

out vec4 out_color;

vec4 select_color()
{
    return (world_z >= out_of_bed_threshold_z) ? uniform_color : vec4(mix(uniform_color.rgb, vec3(0.0), 0.333), uniform_color.a);
}

void main()
{
    vec4 color = select_color();
    out_color = vec4(vec3(intensity.y) + color.rgb * intensity.x, color.a);
}
