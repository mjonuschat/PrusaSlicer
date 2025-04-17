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

uniform vec4 uniform_color;
uniform float emission_factor;

in vec3 eye_position;
in vec3 eye_normal;

out vec4 out_color;

vec4 lighting_phong()
{
    vec3 normal = normalize(eye_normal);

    // Compute the cos of the angle between the normal and lights direction. The light is directional so the direction is constant for every vertex.
    // Since these two are normalized the cosine is the dot product. We also need to clamp the result to the [0,1] range.
    float NdotL = max(dot(normal, LIGHT_TOP_DIR), 0.0);

    // top light
    float ambient = INTENSITY_AMBIENT;
    float diffuse = LIGHT_TOP_DIFFUSE * NdotL;
    float specular = LIGHT_TOP_SPECULAR * pow(max(dot(-normalize(eye_position.xyz), reflect(-LIGHT_TOP_DIR, normal)), 0.0), LIGHT_TOP_SHININESS);
    float emission = emission_factor;

    // front light
    ambient += LIGHT_FRONT_DIFFUSE * max(dot(normal, LIGHT_FRONT_DIR), 0.0);

    return vec4(uniform_color.rgb * (ambient + diffuse + specular + emission), uniform_color.a);
}

void main()
{
    out_color = lighting_phong();
}