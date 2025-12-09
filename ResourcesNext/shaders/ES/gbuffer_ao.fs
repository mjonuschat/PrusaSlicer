#version 100

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

uniform vec4 uniform_color;
uniform int material_id;
uniform PrintVolumeDetection print_volume;

varying vec3 eye_normal;
varying vec4 light_position;
varying vec3 world_position;

layout (location = 0) out vec4 g_light_position;
layout (location = 1) out vec4 g_eye_normal;
layout (location = 2) out vec4 g_color;

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

void main()
{
    g_eye_normal.xyz = normalize(eye_normal);
    g_eye_normal.w = material_id;
    g_light_position = light_position;
    g_color = select_color();
}