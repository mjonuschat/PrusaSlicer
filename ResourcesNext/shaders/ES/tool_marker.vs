#version 300 es

const int MAX_LIGHTS=4;

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
uniform vec3 world_origin;
uniform float scale_factor;
uniform vec4 color_base;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];

in vec3 v_position;
in vec3 v_normal;
out vec4 color;

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
    vec3 world_position = scale_factor * v_position + world_origin;
    vec3 eye_position = (view_model_matrix * vec4(world_position, 1.0)).xyz;
    vec3 eye_normal = normalize(view_normal_matrix * v_normal);
    color = vec4(color_base.rgb * lighting(eye_position, eye_normal), color_base.a);
    gl_Position = projection_matrix * vec4(eye_position, 1.0);
}
