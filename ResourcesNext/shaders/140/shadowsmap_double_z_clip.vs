#version 140

uniform mat4 view_model_matrix;
uniform mat4 projection_matrix;
uniform mat4 volume_world_matrix;

out float world_z;

in vec3 v_position;

void main()
{
    gl_Position = projection_matrix * view_model_matrix * vec4(v_position, 1.0);
    world_z = (volume_world_matrix * vec4(v_position, 1.0)).z;
}
