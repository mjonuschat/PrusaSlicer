#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/App/Scene/BedRenderHelper.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/Color.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "Slic3r/Assert.hpp"

using Slic3r::Domain::ColorRGBA;

namespace Slic3r::App::Scene {

Render::Material BedMaterials::plate_default_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_PLATE_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::plate_textured_material(const Render::Device& device, const Domain::Bed& bed)
{
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("printbed"))
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
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::contour_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::print_volume_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::model_material(const Render::Device& device)
{
    ColorRGBA color = DEFAULT_BED_MODEL_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::axis_material(const Render::Device& device, uint8_t axis)
{
    ColorRGBA color;
    switch (axis)
    {
    case 0: { color = DEFAULT_BED_X_AXIS_COLOR; break; }
    case 1: { color = DEFAULT_BED_Y_AXIS_COLOR; break; }
    case 2: { color = DEFAULT_BED_Z_AXIS_COLOR; break; }
    default: {
        // unsupported axis
        PANIC("Unsupported axis");
    }
    }
    Render::Material ret;
    ret.set_shader(device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::label_material(const Render::Device& device, const std::string& label)
{
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat_texture"))
        .set_texture(0, BedRenderHelper::texture(label, device.context().texture_manager(), Domain::ColorRGB::ORANGE()))
        .set_transparent(true);
    return ret;
}

Render::Material BedMaterials::plate_default_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_PLATE_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::plate_textured_override_material(const Render::Material& primary_material)
{
    Render::Material ret = primary_material;
    ret
        .set_uniform("transparent_background", true)
        .set_transparent(true);
    return ret;
}

Render::Material BedMaterials::grid_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_GRID_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::contour_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::print_volume_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_CONTOUR_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::model_override_material(const Render::Device& device)
{
    ColorRGBA color = DISABLED_BED_MODEL_COLOR;
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());
    return ret;
}

Render::Material BedMaterials::label_override_material(const Render::Device& device, const std::string& label)
{
    const Slic3r::Domain::ColorRGB color{0.8f, 0.8f, 0.8f};
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat_texture"))
        .set_texture(0, BedRenderHelper::texture(label, device.context().texture_manager(), color))
        .set_transparent(true);
    return ret;
}

Render::Material BedMaterials::label_secondary_selection_material(const Render::Device& device, const std::string& label)
{
    const Slic3r::Domain::ColorRGB color{0.6f, 0.6f, 0.6f};
    Render::Material ret;
    ret
        .set_shader(device.context().shader_manager().shader("flat_texture"))
        .set_texture(0, BedRenderHelper::texture(label, device.context().texture_manager(), color))
        .set_transparent(true);
    return ret;
}

} // namespace Slic3r::App::Scene
