///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"

namespace Slic3r::App {

CategoryPageTransformer::CategoryPageTransformer()
{
    set_transform_fn([](const Domain::ConfigItem& data) {
        const Domain::ConfigItemDef::Category category = data.def().category;

        Render::Icon icon = Render::Icon::None;
        switch (category) {
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
            icon = Render::Icon::Notes;
            break;
        case Domain::ConfigItemDef::Category::CustomGcode:
        case Domain::ConfigItemDef::Category::MachineLimits:
            icon = Render::Icon::Cog;
            break;
        case Domain::ConfigItemDef::Category::Filament:
            icon = Render::Icon::FilamentIconMarker;
            break;
        case Domain::ConfigItemDef::Category::SupportMaterial:
        case Domain::ConfigItemDef::Category::Supports:
            icon = Render::Icon::Support;
            break;
        case Domain::ConfigItemDef::Category::General:
            icon = Render::Icon::PrintIconMarker;
            break;
        case Domain::ConfigItemDef::Category::Cooling:
            icon = Render::Icon::Fan;
            break;
        default:
            break;
        }

        return PageEntry{Domain::ConfigItemDef::translate_category(category), icon};
    });
}

} // namespace Slic3r::App
