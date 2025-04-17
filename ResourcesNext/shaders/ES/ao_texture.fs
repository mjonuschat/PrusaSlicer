#version 100

uniform float radius;
uniform float bias;
uniform int kernel_size;
uniform vec2 viewport_size;
uniform mat4 projection_matrix;
uniform vec3 kernel[256];

uniform sampler2D g_eye_position;
uniform sampler2D g_eye_normal;
uniform sampler2D tex_noise;

varying vec2 tex_coord;

void main()
{
    // get input for SSAO algorithm
    vec3 eye_position = texture(g_eye_position, tex_coord).xyz;
    vec3 eye_normal = normalize(texture(g_eye_normal, tex_coord).xyz);
    vec2 tex_noise_size = textureSize(tex_noise, 0);
    vec2 noise_scale = viewport_size / tex_noise_size; 
    vec3 random = normalize(texture(tex_noise, tex_coord * noise_scale).xyz);

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(random - eye_normal * dot(random, eye_normal));
    vec3 bitangent = cross(eye_normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, eye_normal);

    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < kernel_size; ++i) {
        // get sample position
        vec3 sample_pos = TBN * kernel[i]; // from tangent to view-space
        sample_pos = eye_position + sample_pos * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(sample_pos, 1.0);
        offset = projection_matrix * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sample_depth = texture(g_eye_position, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float range_check = smoothstep(0.0, 1.0, radius / abs(eye_position.z - sample_depth));
        occlusion += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;           
    }
    occlusion = 1.0 - (occlusion / kernel_size);
    
    gl_FragColor = occlusion;
}