///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemSubstitutions.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemSubstitutions::ConfigItemSubstitutions(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_preset_interactor(preset_interactor),
    m_cbi_index(cbi_index)
{}

void ConfigItemSubstitutions::on_data_update() {}

} // namespace Slic3r::App
