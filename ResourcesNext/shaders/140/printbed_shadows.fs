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

const vec3 back_color_dark  = vec3(0.235, 0.235, 0.235);
const vec3 back_color_light = vec3(0.365, 0.365, 0.365);

uniform float shadows_intensity;
uniform bool transparent_background;
uniform bool svg_source;

uniform sampler2D in_texture;
uniform sampler2D shadowsmap;

in vec2 tex_coord;
in vec3 eye_position;
in vec3 eye_normal;
in vec4 light_position;

out vec4 out_color;

vec4 svg_color()
{
    // takes foreground from texture
    vec4 fore_color = texture(in_texture, tex_coord);

    // calculates radial gradient
    vec3 back_color = vec3(mix(back_color_light, back_color_dark, smoothstep(0.0, 0.5, length(abs(tex_coord.xy) - vec2(0.5)))));

    // blends foreground with background
    return vec4(mix(back_color, fore_color.rgb, fore_color.a), transparent_background ? fore_color.a : 1.0);
}

vec4 non_svg_color()
{
    // takes foreground from texture
    vec4 color = texture(in_texture, tex_coord);
    return vec4(color.rgb, transparent_background ? color.a * 0.25 : color.a);
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

    // Compute the cos of the angle between the normal and lights direction. The light is directional so the direction is constant for every vertex.
    // Since these two are normalized the cosine is the dot product. We also need to clamp the result to the [0,1] range.
    float NdotL = max(dot(normal, LIGHT_TOP_DIR), 0.0);

    float shadow = shadow_pcf(light_position, NdotL);

    // top light
    float ambient = INTENSITY_AMBIENT;
    float diffuse = shadow * LIGHT_TOP_DIFFUSE * NdotL;
    float specular = shadow * LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(eye_position.xyz), reflect(-LIGHT_TOP_DIR, normal)), 0.0), LIGHT_TOP_SHININESS);

    // front light
    ambient += LIGHT_FRONT_DIFFUSE * max(dot(normal, LIGHT_FRONT_DIR), 0.0);

    vec4 color = svg_source ? svg_color() : non_svg_color();
    color.a = transparent_background ? color.a * 0.5 : color.a;
    return vec4(color.rgb * (ambient + diffuse + specular), color.a);
}

void main()
{
    out_color = lighting_phong();
}