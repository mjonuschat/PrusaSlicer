///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PhysicalPrinterSettingsButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PhysicalPrinterSettingsButton::PhysicalPrinterSettingsButton(
    size_t index, const PhysicalPrinter& physical_printer, FnIndexClicked on_clicked
)
    : Biz::DataObserver<PhysicalPrinter>(index, physical_printer), m_on_clicked(on_clicked)
{
    on_data_update();
    set_flex_shrink(0);

    callbacks().action = [this]() {
        m_on_clicked(m_index);
    };
}

void PhysicalPrinterSettingsButton::on_data_update()
{
    set_printer_name(m_state->m_family + " / " + m_state->m_name);
    set_icon(Render::Icon::PrinterNEXT);
}

} // namespace Slic3r::App
