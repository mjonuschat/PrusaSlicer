#include "Slic3r/App/Plater/BedMaterials.hpp"
#include "Slic3r/App/Plater/BedRenderHelper.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"

#include <boost/algorithm/string/predicate.hpp>

namespace Slic3r::App::Plater {

Render::Material BedMaterials::plate_default_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_PLATE_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::plate_textured_material(const Render::Device& device, const Domain::Bed& bed)
{
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("printbed"))
        .set_texture(0, BedRenderHelper::texture(bed, device.context().texture_manager()))
        .set_uniform("transparent_background", false)
        .set_uniform("svg_source", boost::algorithm::iends_with(bed.texture_filename(), ".svg"));
    return ret;
}

Render::Material BedMaterials::grid_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_GRID_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::contour_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::print_volume_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::model_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_MODEL_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::plate_default_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_PLATE_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::plate_textured_override_material(const Render::Device& device, const Domain::Bed& bed)
{
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("printbed"))
        .set_texture(0, BedRenderHelper::texture(bed, device.context().texture_manager()))
        .set_uniform("transparent_background", true)
        .set_transparent(true)
        .set_uniform("svg_source", boost::algorithm::iends_with(bed.texture_filename(), ".svg"));
    return ret;
}

Render::Material BedMaterials::grid_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_GRID_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::contour_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::print_volume_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

Render::Material BedMaterials::model_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_MODEL_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().get_shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.a() < 1.0f);
    return ret;
}

} // namespace Slic3r::App::Plater
