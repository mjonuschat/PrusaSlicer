#version 150

const float scaling_factor = 1.5;

uniform mat4 view_model_matrix;
uniform mat4 projection_matrix;

uniform samplerBuffer position_tex;
uniform samplerBuffer height_width_angle_tex;
uniform usamplerBuffer segment_index_tex;

in vec3 v_position;

void main()
{
    int id = int(texelFetch(segment_index_tex, gl_InstanceID).r);
    vec2 height_width = texelFetch(height_width_angle_tex, id).xy;
    vec3 offset = texelFetch(position_tex, id).xyz - vec3(0.0, 0.0, 0.5 * height_width.x);
    height_width *= scaling_factor;
    mat3 scale_matrix = mat3(
        height_width.y, 0.0, 0.0,
        0.0, height_width.y, 0.0,
        0.0, 0.0, height_width.x);
    vec3 final_pos = scale_matrix * v_position + offset;
    gl_Position = projection_matrix * view_model_matrix * vec4(final_pos, 1.0);
}
