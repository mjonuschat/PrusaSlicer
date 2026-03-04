///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/LogicalPrinterSettingsDialog.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App {

PrinterAdvancedSettingsDialog::PrinterAdvancedSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator,
    LogicalPrinterSettingsDialog* logical_printer_settings_dialog
) :
    ConfigSettingsDialog(project_interactor, navigator, "PrinterAdvancedSettingsDialog"),
    m_list_selection_changed_scope(project_interactor.preset_interactor().printer_presets(), *this),
    m_logical_printer_settings_dialog(logical_printer_settings_dialog)
{
    Tab* tab = append_tab(_u8L("Printer"));
    m_config_tabs.emplace_back(
        std::make_unique<ConfigTab>(
            &project_interactor.preset_interactor().printer_cbi(),
            tab,
            project_interactor,
            0
        )
    );

    m_footer->set_gap(5.f);

    m_label_preset_name = m_footer->emplace_back<Text>(std::string{});
    m_label_preset_name->set_align({AlignH::Center, AlignV::Center});

    m_footer->emplace_back<Separator>(Orientation::Vertical);

    add_footer_button(_u8("Compare"), Render::Icon::Compare)->callbacks().action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(
            m_project_interactor->preset_interactor(),
            Domain::Preset::PresetKind::FdmPrinter
        );
    };
    add_footer_button(_u8("Save preset"))->callbacks().action = [&]
    {
        m_project_interactor->preset_interactor().save_user_preset(
            Domain::Preset::PresetKind::FdmPrinter,
            0
        );
    };
}

void PrinterAdvancedSettingsDialog::on_list_selection_changed(Domain::SelectionId new_selection)
{
    if (new_selection == Domain::INVALID_ID) {
        return;
    }

    const Biz::Preset::PresetItem& preset_item =
        m_project_interactor->preset_interactor().printer_presets().items().at(new_selection);

    m_label_preset_name->set_text(preset_item.hw_printer_config_name);
}

void PrinterAdvancedSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(m_logical_printer_settings_dialog);
}

} // namespace Slic3r::App
