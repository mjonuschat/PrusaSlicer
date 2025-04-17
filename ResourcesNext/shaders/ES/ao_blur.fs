#version 100

uniform sampler2D in_tex;
uniform int filter_size;

varying vec2 tex_coord;

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(in_tex, 0));
    float result = 0.0;
    int half_filter_size = filter_size / 2;

    for (int x = -half_filter_size; x < -half_filter_size + filter_size; ++x) {
        for (int y = -half_filter_size; y < -half_filter_size + filter_size; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(in_tex, tex_coord + offset).r;
        }
    }
    gl_FragColor = result / (filter_size * filter_size);
}
