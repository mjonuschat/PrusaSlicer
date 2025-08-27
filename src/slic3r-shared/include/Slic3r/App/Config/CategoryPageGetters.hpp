#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::App {

Render::Icon category_render_icon(const Domain::ConfigItemDef::Category category, const Domain::PrinterTechnology pt);

std::string category_icon_name(const Domain::ConfigItemDef::Category category, const Domain::PrinterTechnology pt);

} // namespace Slic3r::App
