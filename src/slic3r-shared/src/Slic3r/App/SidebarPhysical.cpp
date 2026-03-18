#include "Slic3r/App/SidebarPhysical.hpp"

#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/PhysicalPrinterAdvancedSettingsDialog.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsButton.hpp"
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

    m_physical_printer_advanced_settings_dialog = emplace_back<PhysicalPrinterAdvancedSettingsDialog>(project_interactor, navigator);

    m_physical_printer_settings_dialog = emplace_back<PhysicalPrinterSettingsDialog>(
        project_interactor,
        m_printer_add_dialog,
        m_navigator,
        m_physical_printer_advanced_settings_dialog
    );

    set_min_size({YGUndefined, 60});
    set_orientation(Orientation::Vertical);
    set_gap(10);
    set_flex_shrink(0);

    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button = emplace_back<PhysicalPrinterSettingsButton>(
        0,
        physical_printer,
        [this](size_t index) {},
        [this](size_t index) {},
        [this](size_t index) {}
    );
    m_physical_printer_button->set_visible(true);

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
    m_physical_printer_button->on_cog() = [this]()
    {
        if (m_physical_printer_advanced_settings_dialog->opened()) {
            m_navigator.set_opened_dialog(nullptr);
        } else {
            m_physical_printer_advanced_settings_dialog->attach_to_item(m_physical_printer_settings_dialog->content_item(), Position::Left);
            m_navigator.set_opened_dialog(m_physical_printer_advanced_settings_dialog);
        }
    };
}

PhysicalPrinterSettingsDialog& SidebarPhysical::physical_printer_settings_dialog()
{
    return *m_physical_printer_settings_dialog;
}

PhysicalPrinterAdvancedSettingsDialog& SidebarPhysical::physical_printer_advanced_settings_dialog()
{
    return *m_physical_printer_advanced_settings_dialog;
}

void SidebarPhysical::on_printer_data_changed()
{
    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button->set_state(physical_printer);
    m_physical_printer_button->update();
    m_physical_printer_button->set_visible_bin(false);
}

void SidebarPhysical::on_selected_physical_printer_changed()
{
    PhysicalPrinterInteractor& physical_printer_interactor =
        m_project_interactor.physical_printer_interactor();
    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        physical_printer_interactor.selected_physical_printer_data();

    m_physical_printer_button->set_state(physical_printer);
    m_physical_printer_button->update();
    m_physical_printer_button->set_visible_bin(false);
}

} // namespace Slic3r::App
