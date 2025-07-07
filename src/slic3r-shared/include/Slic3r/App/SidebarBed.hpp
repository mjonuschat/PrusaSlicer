///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/MaterialState.hpp"
#include "Slic3r/App/PrinterSettingsDialog.hpp"
#include "Slic3r/App/FilamentSettingsDialog.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"

namespace Slic3r::App {

namespace Yoga {
class Text;
class PrinterSettingsButton;
class MaterialSettingsButton;
} // namespace Yoga

class SidebarBed : public Yoga::Window
{
public:
    explicit SidebarBed();

private:
    Yoga::Text* m_bed_name{nullptr};
    Yoga::PrinterSettingsButton* m_physical_printer_button{nullptr};
    Yoga::PrinterSettingsButton* m_logical_printer_button{nullptr};

    std::shared_ptr<Yoga::ButtonGroup> m_filament_button_group;
    using MaterialListView = Yoga::ListView<
        Yoga::MaterialSettingsButton,
        MaterialState,
        Yoga::ViewFactory<Yoga::MaterialSettingsButton, MaterialState, std::weak_ptr<Yoga::ButtonGroup>>>;
    Biz::ObservableList<MaterialState>
        m_observable_list; ///< this will be moved to more appropriate place

    MaterialListView* m_list_view{nullptr};
    PrinterSettingsDialog m_printer_settings_dialog;
    FilamentSettingsDialog m_filament_settings_dialog;
    PhysicalPrinterSettingsDialog m_physical_printer_settings_dialog;
    PrinterAddDialog m_printer_add_dialog;
};

} // namespace Slic3r::App
