#version 100

precision highp float;

const float THRESHOLD_Z = -0.15;

uniform vec4 uniform_color;
uniform bool enable_out_of_bed_detection_z;

// x = tainted, y = specular;
varying vec2 intensity;
varying float world_z;

vec4 select_color()
{
    return (!enable_out_of_bed_detection_z || world_z >= THRESHOLD_Z) ? uniform_color : vec4(mix(uniform_color.rgb, vec3(0.0), 0.333), uniform_color.a);
}

void main()
{
    vec4 color = select_color();
    gl_FragColor = vec4(vec3(intensity.y) + color.rgb * intensity.x, color.a);
}
