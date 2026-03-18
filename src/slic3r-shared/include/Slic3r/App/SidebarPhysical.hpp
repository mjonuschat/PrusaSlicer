#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {
class Navigator;
class PhysicalPrinterSettingsDialog;
class PrinterAddDialog;
class PhysicalPrinterAdvancedSettingsDialog;
class PhysicalPrinterSettingsButton;

namespace Yoga {
class Text;
} // namespace Yoga

class SidebarPhysical :
    public Yoga::Window,
    public Biz::PhysicalPrinter::IPhysicalPrinterChangedListener
{
public:
    explicit SidebarPhysical(Biz::ProjectInteractor& project_interactor, Navigator& navigator);

    PhysicalPrinterSettingsDialog& physical_printer_settings_dialog();
    PhysicalPrinterAdvancedSettingsDialog& physical_printer_advanced_settings_dialog();

    void on_printer_data_changed() override;

    void on_selected_physical_printer_changed() override;

private:
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;

    PhysicalPrinterSettingsButton* m_physical_printer_button{nullptr};
    PrinterAddDialog* m_printer_add_dialog{nullptr};
    PhysicalPrinterSettingsDialog* m_physical_printer_settings_dialog{nullptr};
    PhysicalPrinterAdvancedSettingsDialog* m_physical_printer_advanced_settings_dialog{nullptr};
};
} // namespace Slic3r::App
