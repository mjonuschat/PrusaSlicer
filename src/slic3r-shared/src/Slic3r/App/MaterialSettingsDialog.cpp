///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/MaterialSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/MaterialSelectionDialog.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;
using namespace Slic3r::Biz;

namespace Slic3r::App {

MaterialSettingsDialog::MaterialSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator,
    MaterialSelectionDialog* material_selection_dialog
) :
    ConfigSettingsDialog(project_interactor, navigator, "MaterialSettingsDialog"),
    m_material_selection_dialog(material_selection_dialog),
    m_material_cbi_list(m_project_interactor.preset_interactor().material_cbi_list())
{
    m_material_cbi_list.add_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);

    Item* footer_items = m_footer->emplace_back<Item>();
    footer_items->set_gap(5.f);
    footer_items->emplace_back<LayoutButton>(_u8("Compare"), Render::Icon::Compare)
        ->callbacks()
        .action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(
            m_project_interactor.preset_interactor(),
            Domain::Preset::PresetKind::FdmMaterial
        );
    };
    footer_items->emplace_back<LayoutButton>(_u8("Save preset"));

    on_reset();
}

MaterialSettingsDialog::~MaterialSettingsDialog()
{
    m_material_cbi_list.remove_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);
}

void MaterialSettingsDialog::on_reset()
{
    Tabs::const_iterator current_tab_it = std::find_if(
        m_tabs.cbegin(),
        m_tabs.cend(),
        [this](const std::unique_ptr<Tab>& tab) { return tab.get() == m_current_tab; }
    );

    std::optional<size_t> current_index;
    if (current_tab_it != m_tabs.cend()) {
        current_index = std::distance(m_tabs.cbegin(), current_tab_it);
    }

    for (size_t tab_index = 0; tab_index < m_config_tabs.size(); ++tab_index) {
        remove_tab(0);
    }
    m_config_tabs.clear();

    for (size_t material_cbi_index = 0; material_cbi_index < m_material_cbi_list.size();
         ++material_cbi_index)
    {
        Biz::ConfigBoxInteractor& cbi =
            const_cast<Biz::ConfigBoxInteractor&>(m_material_cbi_list.at(material_cbi_index));

        std::string tab_name = m_project_interactor.selected_config_container().print_technology()
                == Domain::PrinterTechnology::FFF ?
            _u8L("Filament") :
            _u8L("Material");

        Tab* tab = append_tab(fmt::format("{} {}", tab_name, material_cbi_index + 1));
        m_config_tabs.emplace_back(
            std::make_unique<ConfigTab>(&cbi, tab, m_project_interactor, material_cbi_index)
        );
    }

    if (current_index.has_value()) {
        set_current_tab(current_index.value());
    }
}

void MaterialSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(m_material_selection_dialog);
}

} // namespace Slic3r::App
