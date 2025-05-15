#version 140

uniform vec4 uniform_color;

// x = tainted, y = specular;
in vec2 intensity;

out vec4 out_color;

void main()
{
    out_color = vec4(vec3(intensity.y) + uniform_color.rgb * intensity.x, uniform_color.a);
}
