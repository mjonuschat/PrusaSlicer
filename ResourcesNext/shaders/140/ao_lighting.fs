#version 140

#define INTENSITY_CORRECTION 0.6

// normalized values for (-0.6/1.31, 0.6/1.31, 1./1.31)
const vec3 LIGHT_TOP_DIR = vec3(-0.4574957, 0.4574957, 0.7624929);
#define LIGHT_TOP_DIFFUSE    (0.8 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SPECULAR   (0.125 * INTENSITY_CORRECTION)
#define LIGHT_TOP_SHININESS  20.0

// normalized values for (1./1.43, 0.2/1.43, 1./1.43)
const vec3 LIGHT_FRONT_DIR = vec3(0.6985074, 0.1397015, 0.6985074);
#define LIGHT_FRONT_DIFFUSE  (0.3 * INTENSITY_CORRECTION)

#define INTENSITY_AMBIENT    0.3

#define PI 3.1415926535897932384626433832795

struct Material
{
    float metal;
    float roughness;
    float f0;
};

uniform bool apply_pbr;
uniform float pbr_intensity;
uniform bool apply_shadows;
uniform float shadows_intensity;
uniform Material material;

uniform sampler2D g_eye_position;
uniform sampler2D g_light_position;
uniform sampler2D g_eye_normal;
uniform sampler2D g_color;
uniform sampler2D g_eye_depth;
uniform sampler2D ssao;
uniform sampler2D shadowsmap;

in vec2 tex_coord;

out vec4 out_color;

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
    // Compute the cos of the angle between the normal and lights direction. The light is directional so the direction is constant for every vertex.
    // Since these two are normalized the cosine is the dot product. We also need to clamp the result to the [0,1] range.
    vec3 eye_normal = texture(g_eye_normal, tex_coord).xyz;
    float NdotL = max(dot(eye_normal, LIGHT_TOP_DIR), 0.0);

    vec4 light_position = texture(g_light_position, tex_coord);
    float shadow = apply_shadows ? shadow_pcf(light_position, NdotL) : 1.0;

    // top light
    float ao = texture(ssao, tex_coord).r;
    float ambient = ao * INTENSITY_AMBIENT;
    float diffuse = shadow * LIGHT_TOP_DIFFUSE * NdotL;
    vec3 eye_position = texture(g_eye_position, tex_coord).xyz;
    float specular = shadow * LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(eye_position.xyz), reflect(-LIGHT_TOP_DIR, eye_normal)), 0.0), LIGHT_TOP_SHININESS);

    // front light
    ambient += LIGHT_FRONT_DIFFUSE * max(dot(eye_normal, LIGHT_FRONT_DIR), 0.0);

    vec4 color = texture(g_color, tex_coord);
    return vec4(color.rgb * (ambient + diffuse + specular), color.a);
}

float distribution_ggx(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(n, h), 0.0);

    float nom = a2;
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float geometry_schlick_ggx(float cos_theta, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = cos_theta;
    float denom = cos_theta * (1.0 - k) + k;
    return nom / denom;
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

vec3 light_radiance(vec3 F0, vec3 v, vec3 n, vec3 l, float diffuse, vec3 color)
{
    vec3 h = normalize(v + l);
    vec3 radiance = vec3(diffuse);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(n, h, material.roughness);
    float G = geometry_smith(n, v, l, material.roughness);
    vec3 F = fresnel_schlick(clamp(dot(h, v), 0.0, 1.0), F0);

    vec3 num = NDF * G * F; 
    float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
    vec3 specular = num / denom;

    // kS is equal to Fresnel
    vec3 kS = F;
    // energy conservation
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - material.metal;

    // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    return (kD * color / PI + specular) * radiance * max(dot(n, l), 0.0);
}

vec4 lighting_pbr()
{
    vec3 v = normalize(-texture(g_eye_position, tex_coord).xyz);
    vec3 n = texture(g_eye_normal, tex_coord).xyz;

    vec4 color = texture(g_color, tex_coord);
    color.xyz = pow(color.xyz, vec3(2.2));
    vec3 F0 = mix(vec3(material.f0), color.rgb, material.metal);

    float shadow = apply_shadows ? shadow_pcf(texture(g_light_position, tex_coord), max(dot(n, LIGHT_TOP_DIR), 0.0)) : 1.0;

    vec3 lo = shadow * light_radiance(F0, v, n, LIGHT_TOP_DIR, pbr_intensity * LIGHT_TOP_DIFFUSE, color.rgb);
    lo += light_radiance(F0, v, n, LIGHT_FRONT_DIR, pbr_intensity * LIGHT_FRONT_DIFFUSE, color.rgb);

    vec3 ambient = vec3(2.0f * INTENSITY_AMBIENT) * color.rgb * texture(ssao, tex_coord).r;

    vec3 pbr_color = ambient + lo;

    // HDR tonemapping
    pbr_color = pbr_color / (pbr_color + vec3(1.0));
//    // gamma correct
//    pbr_color = pow(pbr_color, vec3(1.0/2.2));

    return vec4(pbr_color, color.a);
}

void main()
{
    out_color = apply_pbr ? lighting_pbr() : lighting_phong();
}
