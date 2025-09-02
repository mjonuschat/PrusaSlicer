#version 330

uniform mat4 projection_view_model_matrix;
uniform mat3 view_normal_matrix;
uniform mat4 light_matrix;

in vec3 v_position;
in vec3 v_normal;

out vec4 light_position;
out vec3 eye_normal;

void main()
{
    eye_normal = view_normal_matrix * v_normal;
    light_position = light_matrix * vec4(v_position, 1.0);
    gl_Position = projection_view_model_matrix * vec4(v_position, 1.0);
}