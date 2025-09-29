#version 100

uniform mat4 model_matrix;
uniform mat4 projection_view_model_matrix;
uniform mat3 view_normal_matrix;
uniform mat4 light_matrix;

attribute vec3 v_position;
attribute vec3 v_normal;

varying vec3 eye_normal;
varying vec4 light_position;
varying vec3 world_position;

void main()
{
    eye_normal = view_normal_matrix * v_normal;
    light_position = light_matrix * vec4(v_position, 1.0);
    world_position = (model_matrix * vec4(v_position, 1.0)).xyz;
    gl_Position = projection_view_model_matrix * vec4(v_position, 1.0);
}