#version 100

precision highp float;

uniform vec4 uniform_color;

// x = tainted, y = specular;
varying vec2 intensity;

void main()
{
    gl_FragColor = vec4(vec3(intensity.y) + uniform_color.rgb * intensity.x, uniform_color.a);
}
