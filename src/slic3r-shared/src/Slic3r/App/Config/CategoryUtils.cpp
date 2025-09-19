#include "Slic3r/App/Config/CategoryUtils.hpp"
#include "Slic3r/App/Render/ImguiIconHelper.hpp"

namespace Slic3r::App {

namespace CategoryUtils {

Render::Icon category_render_icon(
    const Domain::ConfigItemDef::Category category,
    const Domain::PrinterTechnology pt
)
{
    Render::Icon icon = Render::Icon::None;

    switch (category) {
    case Domain::ConfigItemDef::Category::General:
        icon = pt == Domain::PrinterTechnology::FFF ? Render::Icon::PrinterIconMarker :
                                                      Render::Icon::PrinterSlaIconMarker;
        break;
    case Domain::ConfigItemDef::Category::Material:
        icon = pt == Domain::PrinterTechnology::FFF ? Render::Icon::FilamentIconMarker :
                                                      Render::Icon::MaterialIconMarker;
        break;
    case Domain::ConfigItemDef::Category::Advanced:
        icon = Render::Icon::Cogs;
        break;
    case Domain::ConfigItemDef::Category::LayersAndPerimeters:
        icon = Render::Icon::Layers;
        break;
    case Domain::ConfigItemDef::Category::Infill:
        icon = Render::Icon::Infill;
        break;
    case Domain::ConfigItemDef::Category::SkirtAndBrim:
        icon = Render::Icon::SkirtBrim;
        break;
    case Domain::ConfigItemDef::Category::Speed:
        icon = Render::Icon::Time;
        break;
    case Domain::ConfigItemDef::Category::Extruders:
    case Domain::ConfigItemDef::Category::MultipleExtruders:
        icon = Render::Icon::Funnel;
        break;
    case Domain::ConfigItemDef::Category::OutputOptions:
        icon = Render::Icon::Output;
        break;
    case Domain::ConfigItemDef::Category::Notes:
    case Domain::ConfigItemDef::Category::MaterialPrintingProfile:
        icon = Render::Icon::Notes;
        break;
    case Domain::ConfigItemDef::Category::CustomGcode:
    case Domain::ConfigItemDef::Category::MachineLimits:
        icon = Render::Icon::Cog;
        break;
    case Domain::ConfigItemDef::Category::SupportMaterial:
    case Domain::ConfigItemDef::Category::Supports:
        icon = Render::Icon::Support;
        break;
    case Domain::ConfigItemDef::Category::Cooling:
        icon = Render::Icon::Fan;
        break;
    case Domain::ConfigItemDef::Category::SingleExtruderMMSetup:
        icon = Render::Icon::PrinterIconMarker;
        break;
    default:
        break;
    }

    return icon;
}

std::string category_icon_name(
    const Domain::ConfigItemDef::Category category,
    const Domain::PrinterTechnology pt
)
{
    Render::Icon icon = category_render_icon(category, pt);
    return Render::ImguiIconHelper::icon_name(icon);
}

} // namespace CategoryUtils

} // namespace Slic3r::App
