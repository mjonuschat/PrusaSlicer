#version 300 es

precision lowp usampler2D;

const int MAX_LIGHTS=4;
const float scaling_factor = 1.5;

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
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform sampler2D position_tex;
uniform sampler2D height_width_angle_tex;
uniform sampler2D color_tex;
uniform usampler2D segment_index_tex;

in vec3 v_position;
in vec3 v_normal;
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

ivec2 tex_coord(sampler2D sampler, int id) {
    ivec2 tex_size = textureSize(sampler, 0);
    return (tex_size.y == 1) ? ivec2(id, 0) : ivec2(id % tex_size.x, id / tex_size.x);
}

ivec2 tex_coord_u(usampler2D sampler, int id) {
    ivec2 tex_size = textureSize(sampler, 0);
    return (tex_size.y == 1) ? ivec2(id, 0) : ivec2(id % tex_size.x, id / tex_size.x);
}

void main() {
    int id = int(texelFetch(segment_index_tex, tex_coord_u(segment_index_tex, gl_InstanceID), 0).r);
    vec2 height_width = texelFetch(height_width_angle_tex, tex_coord(height_width_angle_tex, id), 0).xy;
    vec3 offset = texelFetch(position_tex, tex_coord(position_tex, id), 0).xyz - vec3(0.0, 0.0, 0.5 * height_width.x);
    height_width *= scaling_factor;
    mat3 scale_matrix = mat3(
        height_width.y, 0.0, 0.0,
        0.0, height_width.y, 0.0,
        0.0, 0.0, height_width.x);
    vec3 eye_position = (view_model_matrix * vec4(scale_matrix * v_position + offset, 1.0)).xyz;
    vec3 eye_normal = normalize(view_normal_matrix * v_normal);
    vec3 color_base = decode_color(texelFetch(color_tex, tex_coord(color_tex, id), 0).r);
    color = color_base * lighting(eye_position, eye_normal);
    gl_Position = projection_matrix * vec4(eye_position, 1.0);
}
