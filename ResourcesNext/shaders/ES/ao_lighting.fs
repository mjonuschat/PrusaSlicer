#version 100

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

struct Material
{
    float metal;
    float roughness;
    float ior;
};

uniform bool apply_pbr;
uniform float pbr_intensity;
uniform bool apply_shadows;
uniform float shadows_intensity;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform mat4 view_matrix;
uniform float ambient_intensity;

uniform sampler2D g_eye_position;
uniform sampler2D g_light_position;
uniform sampler2D g_eye_normal;
uniform sampler2D g_color;
uniform sampler2D g_material;
uniform sampler2D g_eye_depth;
uniform sampler2D ssao;
uniform sampler2D shadowsmap;

varying vec2 tex_coord;

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

vec3 light_direction(Light light)
{
    // return light direction in eye coordinates
    return (light.system == 0) ? (view_matrix * vec4(-light.direction, 0.0)).xyz : -light.direction;
}

vec4 lighting_phong()
{
    vec3 eye_normal = texture(g_eye_normal, tex_coord).xyz;
    vec3 eye_position = texture(g_eye_position, tex_coord).xyz;
    vec4 light_position = texture(g_light_position, tex_coord);
    float ao = texture(ssao, tex_coord).r;

    float ambient = 0.0;
    float diffuse = 0.0;
    float specular = 0.0;
    for (int i = 0; i < num_lights; ++i) {
        ambient += ao * lights[i].ambient;
        vec3 dir = light_direction(lights[i]);
        float NdotL = max(dot(eye_normal, dir), 0.0);
        float shadow = (apply_shadows && lights[i].shadows) ? shadow_pcf(light_position, NdotL) : 1.0;
        diffuse += shadow * lights[i].diffuse * NdotL;
        specular += shadow * lights[i].specular * pow(max(dot(-normalize(eye_position), reflect(-dir, eye_normal)), 0.0), lights[i].shininess);
    }

    vec4 color = texture(g_color, tex_coord);
    return vec4(color.rgb * (ambient + diffuse + specular), color.a);
}

float ior_to_f0(float ior)
{
    float num = ior - 1.0f;
    float denom = ior + 1.0f;
    return num * num / (denom * denom);
}

float distribution_ggx(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(n, h), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_ggx(float cos_theta, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    return cos_theta / (cos_theta * (1.0 - k) + k);
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float ggx1 = geometry_schlick_ggx(max(dot(n, v), 0.0), roughness);
    float ggx2 = geometry_schlick_ggx(max(dot(n, l), 0.0), roughness);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 light_radiance(vec3 F0, vec3 v, vec3 n, vec3 l, float diffuse, Material material, vec3 color)
{
    vec3 h = normalize(v + l);
    vec3 radiance = vec3(diffuse);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(n, h, material.roughness);
    float G = geometry_smith(n, v, l, material.roughness);
    vec3 F = fresnel_schlick(clamp(dot(h, v), 0.0, 1.0), F0);

    float NdotL = max(dot(n, l), 0.0);
    float denom = 4.0 * max(dot(n, v), 0.0) * NdotL + 0.0001;
    vec3 specular = NDF * G * F / denom;

    // energy conservation
    vec3 kD = vec3(1.0) - F;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - material.metal;

    // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    return (kD * color / PI + specular) * radiance * NdotL;
}

vec4 lighting_pbr()
{
    vec3 v = normalize(-texture(g_eye_position, tex_coord).xyz);
    vec3 n = texture(g_eye_normal, tex_coord).xyz;
    vec3 m = texture(g_material, tex_coord).xyz;
    Material material;
    material.metal = m.x;
    material.roughness = m.y;
    material.ior = m.z;

    vec4 color = texture(g_color, tex_coord);
    color.xyz = pow(color.xyz, vec3(2.2));
    vec3 F0 = mix(vec3(ior_to_f0(material.ior)), color.rgb, material.metal);

    vec3 lo = vec3(0.0);
    for (int i = 0; i < num_lights; ++i) {
        vec3 dir = light_direction(lights[i]);
        float shadow = (apply_shadows && lights[i].shadows) ? shadow_pcf(texture(g_light_position, tex_coord), max(dot(n, dir), 0.0)) : 1.0;
        lo += shadow * light_radiance(F0, v, n, dir, pbr_intensity * lights[i].diffuse, material, color.rgb);
    }

    vec3 ambient = vec3(ambient_intensity) * color.rgb * texture(ssao, tex_coord).r;
    vec3 pbr_color = ambient + lo;

    // HDR tonemapping
    pbr_color = pbr_color / (pbr_color + vec3(1.0));
//    // gamma correct
//    pbr_color = pow(pbr_color, vec3(1.0/2.2));

    return vec4(pbr_color, color.a);
}

void main()
{
    vec4 color = apply_pbr ? lighting_pbr() : lighting_phong();
    if (color.w == 0.0)
        discard;
    gl_FragColor = color;
}
