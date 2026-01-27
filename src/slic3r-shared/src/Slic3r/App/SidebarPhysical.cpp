#include "Slic3r/App/SidebarPhysical.hpp"

#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/PhysicalPrinterAdvancedSettingsDialog.hpp"
#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"
#include "Slic3r/App/Render/ImguiIconHelper.hpp"

#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterInteractor.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz::PhysicalPrinter;
namespace Slic3r::App {

SidebarPhysical::SidebarPhysical(Biz::ProjectInteractor& project_interactor, Navigator& navigator) :
    Window("SidebarPhysical"),
    m_project_interactor(project_interactor),
    m_navigator(navigator)
{
    m_project_interactor.physical_printer_interactor()
        .add_listener<Biz::PhysicalPrinter::IPhysicalPrinterChangedListener>(this);
    m_printer_add_dialog = emplace_back<PrinterAddDialog>(m_navigator);

    m_physical_printer_settings_dialog = emplace_back<PhysicalPrinterSettingsDialog>(
        project_interactor,
        m_printer_add_dialog,
        m_navigator
    );

    set_min_size({YGUndefined, 60});
    set_orientation(Orientation::Vertical);
    set_gap(10);
    set_flex_shrink(0);

    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button = emplace_back<Yoga::PrinterSettingsButton>("Physical printer");
    m_physical_printer_button->set_printer_name(physical_printer.name);
    m_physical_printer_button->set_preset_name(Biz::PhysicalPrinter::physical_printer_type_to_string(physical_printer));
    m_physical_printer_button->set_visible(true);
    m_physical_printer_button->set_icon(Render::Icon::PrinterIconMarker);

    m_physical_printer_settings_dialog->attach_to_item(this, Position::Left);
    m_physical_printer_settings_dialog->callbacks().opened = [this]()
    { m_physical_printer_button->set_checked(true); };
    m_physical_printer_settings_dialog->callbacks().closed = [this]()
    { m_physical_printer_button->set_checked(false); };

    m_physical_printer_button->callbacks().action = [this]()
    {
        if (m_physical_printer_settings_dialog->opened()) {
            m_navigator.set_opened_dialog(nullptr);
        } else {
            m_navigator.set_opened_dialog(m_physical_printer_settings_dialog);
        }
    };
}

PhysicalPrinterSettingsDialog& SidebarPhysical::physical_printer_settings_dialog()
{
    return *m_physical_printer_settings_dialog;
}

PhysicalPrinterAdvancedSettingsDialog& SidebarPhysical::print_host_settings_dialog()
{
    return m_physical_printer_settings_dialog->print_host_settings_dialog();
}

void SidebarPhysical::on_printer_data_changed()
{
    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button->set_printer_name(physical_printer.name);
    m_physical_printer_button->set_preset_name(
        std::string(Biz::PhysicalPrinter::physical_printer_type_to_string(physical_printer))
    );
}

void SidebarPhysical::on_selected_physical_printer_changed()
{
    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button->set_printer_name(physical_printer.name);
    m_physical_printer_button->set_preset_name(
        std::string(Biz::PhysicalPrinter::physical_printer_type_to_string(physical_printer))
    );
}

} // namespace Slic3r::App
