///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrintSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Navigator.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;
using namespace Slic3r::Biz;

namespace Slic3r::App {

PrintSettingsDialog::PrintSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator
) :
    ConfigSettingsDialog(project_interactor, navigator, "PrintSettingsDialog"),
    m_tool_cbi_list(project_interactor.preset_interactor().tool_cbi_list())
{
    Tab* tab = append_tab(_u8L("Print"));
    m_config_tabs.emplace_back(
        std::make_unique<ConfigTab>(
            &project_interactor.preset_interactor().print_cbi(),
            tab,
            project_interactor,
            0
        )
    );

    Item* footer_items = m_footer->emplace_back<Item>();
    footer_items->set_gap(5.f);
    footer_items->emplace_back<LayoutButton>(_u8("Compare"), Render::Icon::Compare)
        ->callbacks()
        .action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(
            m_project_interactor->preset_interactor(),
            Domain::Preset::PresetKind::FdmPrint
        );
    };
    footer_items->emplace_back<LayoutButton>(_u8("Save preset"));

    m_tool_cbi_list.add_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);

    on_reset();
}

PrintSettingsDialog::~PrintSettingsDialog()
{
    m_tool_cbi_list.remove_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);
}

void PrintSettingsDialog::on_reset()
{
    while (m_tabs.size() > 1) {
        remove_tab(1);
    }

    for (size_t tool_cbi_index = 0; tool_cbi_index < m_tool_cbi_list.size(); ++tool_cbi_index) {
        Biz::ConfigBoxInteractor& cbi =
            const_cast<Biz::ConfigBoxInteractor&>(m_tool_cbi_list.at(tool_cbi_index));

        Tab* tab = append_tab(fmt::format("Tool {}", tool_cbi_index + 1));
        m_config_tabs.emplace_back(
            std::make_unique<ConfigTab>(&cbi, tab, *m_project_interactor, tool_cbi_index)
        );
    }
}

void PrintSettingsDialog::navigate_to_item(const Domain::ConfigItem* config_item)
{
    ConfigTabPtr* tab = std::visit(
        [this](auto&& location) -> ConfigTabPtr*
        {
            using T = std::decay_t<decltype(location)>;
            if constexpr (std::is_same_v<T, Domain::FDMConfigLocation>) {
                if (location == Domain::FDMConfigLocation::Print) {
                    return &m_config_tabs.at(0);
                } else {
                    return &m_config_tabs.at(1);
                }
            } else if constexpr (std::is_same_v<T, Domain::SLAConfigLocation>) {
                if (location == Domain::SLAConfigLocation::Print) {
                    return &m_config_tabs.at(0);
                }
            }

            return nullptr;
        },
        config_item->location()
    );

    if (tab) {
        set_current_tab(
            std::distance(
                m_config_tabs.cbegin(),
                std::find_if(
                    m_config_tabs.cbegin(),
                    m_config_tabs.cend(),
                    [tab](const ConfigTabPtr& search_tab) { return *tab == search_tab; }
                )
            )
        );
        (*tab)->navigate_to_item(config_item);
    }
}

void PrintSettingsDialog::clear_navigation()
{
    m_config_tabs.at(0)->clear_navigation();
    if (m_config_tabs.size() > 1) { // SLA does not have Tool page
        m_config_tabs.at(1)->clear_navigation();
    }
}

void PrintSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

} // namespace Slic3r::App
