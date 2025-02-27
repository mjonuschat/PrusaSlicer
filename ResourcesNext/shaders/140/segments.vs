#version 150

#define POINTY_CAPS
#define FIX_TWISTING

const int MAX_LIGHTS=4;
const vec3 UP = vec3(0, 0, 1);

struct Light
{
    int system;
    vec3 direction;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

uniform mat4 view_model_matrix;
uniform mat4 projection_matrix;
uniform mat3 view_normal_matrix;
uniform vec3 camera_position;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform samplerBuffer position_tex;
uniform samplerBuffer height_width_angle_tex;
uniform samplerBuffer color_tex;
uniform usamplerBuffer segment_index_tex;

in int v_position;
out vec3 color;

vec3 decode_color(float color) {
    int c = int(round(color));
    int r = (c >> 16) & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = (c >> 0) & 0xFF;
    float f = 1.0 / 255.0f;
    return f * vec3(r, g, b);
}

float lighting(vec3 eye_position, vec3 eye_normal) {
    float ambient = 0.0;
    float diffuse = 0.0;
    float specular = 0.0;
    for (int i = 0; i < num_lights; ++i) {
        vec3 light_direction = (lights[i].system == 0) ? (view_model_matrix * -vec4(lights[i].direction, 0.0f)).xyz : lights[i].direction;
        ambient += lights[i].ambient;
        diffuse += lights[i].diffuse * max(dot(eye_normal, light_direction), 0.0);
        specular += lights[i].specular * pow(max(dot(-normalize(eye_position), reflect(-light_direction, eye_normal)), 0.0), lights[i].shininess);
    };
    return ambient + diffuse + specular;
}

void main() {
    int id_a = int(texelFetch(segment_index_tex, gl_InstanceID).r);
    int id_b = id_a + 1;
    vec3 pos_a = texelFetch(position_tex, id_a).xyz;
    vec3 pos_b = texelFetch(position_tex, id_b).xyz;
    vec3 line = pos_b - pos_a;
    // directions of the line box in world space
    float line_len = length(line);
    vec3 line_dir;
    if (line_len < 1e-4)
        line_dir = vec3(1.0, 0.0, 0.0);
    else
        line_dir = line / line_len;
    vec3 line_right_dir;
    if (abs(dot(line_dir, UP)) > 0.9) {
        // For vertical lines, the width and height should be same, there is no concept of up and down.
        // For simplicity, the code will expand width in the x axis, and height in the y axis
        line_right_dir = normalize(cross(vec3(1, 0, 0), line_dir));
    }
    else
        line_right_dir = normalize(cross(line_dir, UP));
    vec3 line_up_dir = normalize(cross(line_right_dir, line_dir));
    const vec2 horizontal_vertical_view_signs_array[16] = vec2[](
        //horizontal view (from right)
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
        vec2(0.0, -1.0),
        vec2(0.0, -1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
        // vertical view (from top)
        vec2(0.0, 1.0),
        vec2(-1.0, 0.0),
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(-1.0, 0.0),
        vec2(0.0, 0.0)
    );
    int id = v_position < 4 ? id_a : id_b;
    vec3 endpoint_pos = v_position < 4 ? pos_a : pos_b;
    vec3 height_width_angle = texelFetch(height_width_angle_tex, id).xyz;
#ifdef FIX_TWISTING
    int closer_id = (dot(camera_position - pos_a, camera_position - pos_a) < dot(camera_position - pos_b, camera_position - pos_b)) ? id_a : id_b;
    vec3 closer_pos = (closer_id == id_a) ? pos_a : pos_b;
    vec3 camera_view_dir = normalize(closer_pos - camera_position);
    vec3 closer_height_width_angle = texelFetch(height_width_angle_tex, closer_id).xyz;
    vec3 diagonal_dir_border = normalize(closer_height_width_angle.x * line_up_dir + closer_height_width_angle.y * line_right_dir);
#else
    vec3 camera_view_dir = normalize(endpoint_pos - camera_position);
    vec3 diagonal_dir_border = normalize(height_width_angle.x * line_up_dir + height_width_angle.y * line_right_dir);
#endif
    bool is_vertical_view = abs(dot(camera_view_dir, line_up_dir)) / abs(dot(diagonal_dir_border, line_up_dir)) >
        abs(dot(camera_view_dir, line_right_dir)) / abs(dot(diagonal_dir_border, line_right_dir));
    vec2 signs = horizontal_vertical_view_signs_array[v_position + 8 * int(is_vertical_view)];
#ifndef POINTY_CAPS
    if (v_position == 2 || v_position == 7) signs = -horizontal_vertical_view_signs_array[(v_position - 2) + 8 * int(is_vertical_view)];
#endif
    float view_right_sign = sign(dot(-camera_view_dir, line_right_dir));
    float view_top_sign = sign(dot(-camera_view_dir, line_up_dir));
    float half_height = 0.5 * height_width_angle.x;
    float half_width = 0.5 * height_width_angle.y;
    vec3 horizontal_dir = half_width * line_right_dir;
    vec3 vertical_dir = half_height * line_up_dir;
    float horizontal_sign = signs.x * view_right_sign;
    float vertical_sign = signs.y * view_top_sign;
    vec3 pos = endpoint_pos + horizontal_sign * horizontal_dir + vertical_sign * vertical_dir;
    if (v_position == 2 || v_position == 7) {
        float line_dir_sign = (v_position == 2) ? -1.0 : 1.0;
        if (height_width_angle.z == 0) {
#ifdef POINTY_CAPS
            // There I add a cap to lines that do not have a following line
            // (or they have one, but perfectly aligned, so the cap is hidden inside the next line).
            pos += line_dir_sign * line_dir * half_width;
#endif
        }
        else {
            pos += line_dir_sign * line_dir * half_width * sin(abs(height_width_angle.z) * 0.5);
            pos += sign(height_width_angle.z) * horizontal_dir * cos(abs(height_width_angle.z) * 0.5);
        }
    }
    vec3 eye_position = (view_model_matrix * vec4(pos, 1.0)).xyz;
    vec3 eye_normal = normalize(view_normal_matrix * (pos - endpoint_pos));
    vec3 color_base = decode_color(texelFetch(color_tex, id).r);
    color = color_base * lighting(eye_position, eye_normal);
    gl_Position = projection_matrix * vec4(eye_position, 1.0);
}
