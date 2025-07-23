///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/LogicalPrinterSettingsButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

LogicalPrinterSettingsButton::LogicalPrinterSettingsButton(
    size_t index, const Biz::Preset::PresetItem& logical_printer, FnIndexClicked on_clicked
)
    : Biz::DataObserver<Biz::Preset::PresetItem>(index, logical_printer), m_on_clicked(on_clicked)
{
    on_data_update();
    set_flex_shrink(0);

    callbacks().action = [this]() { m_on_clicked(m_index); };
}

void LogicalPrinterSettingsButton::on_data_update()
{
    set_printer_name(m_state->name);
    set_preset_name(m_state->hw_pritner_config_name);
    set_icon(Render::Icon::PrinterNEXT);
}

} // namespace Slic3r::App
