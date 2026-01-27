#include "Slic3r/App/PhysicalPrinterAdvancedSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/LogicalPrinterSettingsDialog.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;
using namespace Slic3r::Biz;

namespace Slic3r::App {

PhysicalPrinterAdvancedSettingsDialog::PhysicalPrinterAdvancedSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator,
    PhysicalPrinterSettingsDialog* physical_printer_settings_dialog
) :
    ConfigSettingsDialog(project_interactor, navigator, "PhysicalPrinterAdvancedSettingsDialog"),
    m_physical_printer_changed_listener_scope(project_interactor.physical_printer_interactor(), *this),
    m_physical_printer_settings_dialog(physical_printer_settings_dialog)
{
    Item* footer_items = m_footer->emplace_back<Item>();
    footer_items->set_gap(5.f);

    footer_items->emplace_back<Separator>(Orientation::Vertical);

    m_save_button = footer_items->emplace_back<LayoutButton>(_u8("Save"));
    m_save_button->callbacks().action = [&]
    {
        m_project_interactor->physical_printer_interactor().save_current_edit();
        m_navigator.set_opened_dialog(m_physical_printer_settings_dialog);
    };

    Tab* tab = append_tab(_u8L("Printer"));
    m_config_tabs.emplace_back(
        std::make_unique<ConfigTab>(
            m_project_interactor->physical_printer_interactor().cbi(),
            tab,
            m_project_interactor->physical_printer_interactor()
        )
    );
}

void PhysicalPrinterAdvancedSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(m_physical_printer_settings_dialog);
}

void PhysicalPrinterAdvancedSettingsDialog::on_selected_physical_printer_changed() 
{
    m_save_button->set_visible(m_project_interactor->physical_printer_interactor().is_none_selected());
}

} // namespace Slic3r::App
