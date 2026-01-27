///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PhysicalPrinterSettingsButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PhysicalPrinterSettingsButton::PhysicalPrinterSettingsButton(
    size_t index, const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer, FnIndexClicked on_clicked
)
    : Biz::DataObserver<Biz::PhysicalPrinter::PhysicalPrinterConfig>(index, physical_printer), m_on_clicked(on_clicked)
{
    on_data_update();
    set_flex_shrink(0);

    callbacks().action = [this]() {
        m_on_clicked(m_index);
    };
}

void PhysicalPrinterSettingsButton::on_data_update()
{
    set_preset_name(Biz::PhysicalPrinter::physical_printer_type_to_string(*m_state));
    set_printer_name(m_state->name);
    if (m_state->operation_type == Biz::PhysicalPrinter::OperationType::None) {
         set_icon(Render::Icon::PrintIdle);
    } else {
        set_icon(Render::Icon::PrinterIconMarker);
    }
}

void PhysicalPrinterSettingsButton::update_button_text()
{
    on_data_update();
}

} // namespace Slic3r::App
