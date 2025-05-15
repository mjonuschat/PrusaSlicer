#version 300 es

#define MAX_LIGHTS 4
#define PI 3.1415926535897932384626433832795

struct Light
{
    int system;
    vec3 direction;
    bool shadows;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

uniform float shadows_intensity;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform mat4 view_matrix;

uniform sampler2D shadowsmap;

in vec3 eye_position;
in vec3 eye_normal;
in vec4 light_position;
in vec4 var_color;

vec3 light_direction(Light light)
{
    // return light direction in eye coordinates
    return (light.system == 0) ? (view_matrix * vec4(-light.direction, 0.0)).xyz : -light.direction;
}

float shadow_pcf(vec4 position, float NdotL)
{
    // perform perspective divide
    vec3 proj_coords = position.xyz / position.w;
    // transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;

    float bias = max(0.01 * (1.0 - NdotL), 0.001);

    // PCF
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(shadowsmap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcf_depth = texture(shadowsmap, proj_coords.xy + vec2(x, y) * texel_size).r;
            shadow += proj_coords.z - bias > pcf_depth ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    
    // if outside the light frustum -> lit
    return (proj_coords.z - bias > 1.0) ? 1.0 : 1.0 - shadows_intensity * shadow;
}

vec4 lighting_phong()
{
    vec3 normal = normalize(eye_normal);

    float ambient = 0.0;
    float diffuse = 0.0;
    float specular = 0.0;
    for (int i = 0; i < num_lights; ++i) {
        ambient += lights[i].ambient;
        vec3 dir = light_direction(lights[i]);
        float NdotL = max(dot(normal, dir), 0.0);
        float shadow = lights[i].shadows ? shadow_pcf(light_position, NdotL) : 1.0;
        diffuse += shadow * lights[i].diffuse * NdotL;
        specular += shadow * lights[i].specular * pow(max(dot(-normalize(eye_position), reflect(-dir, normal)), 0.0), lights[i].shininess);
    }

    return vec4(var_color.rgb * (ambient + diffuse + specular), var_color.a);
}

void main()
{
    gl_FragColor = lighting_phong();
}
