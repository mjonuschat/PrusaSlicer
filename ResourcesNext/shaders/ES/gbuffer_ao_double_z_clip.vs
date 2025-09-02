#version 100

uniform mat4 projection_view_model_matrix;
uniform mat3 view_normal_matrix;
uniform mat4 volume_world_matrix;
uniform mat4 light_matrix;

attribute vec3 v_position;
attribute vec3 v_normal;

varying vec4 light_position;
varying vec3 eye_normal;
varying float world_z;

void main()
{
    eye_normal = view_normal_matrix * v_normal;
    light_position = light_matrix * vec4(v_position, 1.0);
    gl_Position = projection_view_model_matrix * vec4(v_position, 1.0);
    world_z = (volume_world_matrix * vec4(v_position, 1.0)).z;
}