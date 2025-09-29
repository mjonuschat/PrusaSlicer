#version 100

precision highp float;

uniform vec4 uniform_color;
uniform float out_of_bed_threshold_z;

// x = tainted, y = specular;
varying vec2 intensity;
varying float world_z;

vec4 select_color()
{
    return (world_z >= out_of_bed_threshold_z) ? uniform_color : vec4(mix(uniform_color.rgb, vec3(0.0), 0.333), uniform_color.a);
}

void main()
{
    vec4 color = select_color();
    gl_FragColor = vec4(vec3(intensity.y) + color.rgb * intensity.x, color.a);
}
