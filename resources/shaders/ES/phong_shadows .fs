#version 100

#define MAX_LIGHTS 4

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

struct PrintVolumeDetection
{
    // 0 -> Invalid
    // 1 -> Rectangle
    // 2 -> Circle
    // 3 -> Convex
    // 4 -> Custom
    int type;
    // Invalid/Rectangle/Convex/Custom:
    //   x = min x
    //   y = min y
    //   z = max x
    //   w = max y
    // Circle:
    //   x = center x
    //   y = center y
    //   z = radius
    //   w = not used
    vec4 xy_data;
    // x = min z
    // y = max z
    vec2 z_data;
};

const vec3 ZERO = vec3(0.0, 0.0, 0.0);
const int TYPE_INVALID = 0;
const int TYPE_RECTANGLE = 1;
const int TYPE_CIRCLE = 2;

uniform float shadows_intensity;
uniform vec4 uniform_color;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform mat4 view_matrix;
uniform PrintVolumeDetection print_volume;

uniform sampler2D shadowsmap;

varying vec3 eye_normal;
varying vec3 eye_position;
varying vec4 light_position;
varying vec3 world_position;

vec3 light_direction(Light light)
{
    // return light direction in eye coordinates
    return (light.system == 0) ? (view_matrix * vec4(-light.direction, 0.0)).xyz : -light.direction;
}

vec4 select_color()
{
    // if the fragment is outside the print volume -> use darker color
    vec3 pv_check_min = ZERO;
    vec3 pv_check_max = ZERO;
    switch (print_volume.type)
    {
    case TYPE_INVALID:
    {
        // consider as inside
        pv_check_min = vec3(1.0);
        pv_check_max = vec3(-1.0);
        break;
    }
    case TYPE_RECTANGLE:
    {
        pv_check_min = world_position.xyz - vec3(print_volume.xy_data.x, print_volume.xy_data.y, print_volume.z_data.x);
        pv_check_max = world_position.xyz - vec3(print_volume.xy_data.z, print_volume.xy_data.w, print_volume.z_data.y);
        break;
    }
    case TYPE_CIRCLE:
    {
        float delta_radius = print_volume.xy_data.z - distance(world_position.xy, print_volume.xy_data.xy);
        pv_check_min = vec3(delta_radius, 0.0, world_position.z - print_volume.z_data.x);
        pv_check_max = vec3(0.0, 0.0, world_position.z - print_volume.z_data.y);
        break;
    }
    default:
    {
        // check only z
        pv_check_min = vec3(0.0, 0.0, world_position.z - print_volume.z_data.x);
        pv_check_max = vec3(0.0, 0.0, world_position.z - print_volume.z_data.y);
        break;
    }
    }
    return vec4((any(lessThan(pv_check_min, ZERO)) || any(greaterThan(pv_check_max, ZERO))) ? mix(uniform_color.rgb, ZERO, 0.3333) : uniform_color.rgb, uniform_color.a);
}

float shadow_pcf(vec4 position, float NdotL)
{
    // perform perspective divide
    vec3 proj_coords = position.xyz / position.w;
    // transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;

    if (proj_coords.x < 0.0 || proj_coords.x > 1.0 || proj_coords.y < 0.0 || proj_coords.y > 1.0 || proj_coords.z > 1.0)
        // Fully lit
        return 1.0;

    float bias = max(0.0025 * (1.0 - NdotL), 0.00025);

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

    return 1.0 - shadows_intensity * shadow;
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

     vec4 color = select_color();
     return vec4(color.rgb * (ambient + diffuse + specular), color.a);
}

void main()
{
    gl_FragColor = lighting_phong();
}