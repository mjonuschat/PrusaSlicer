///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrintSettingsDialog.hpp"
#include <Slic3r/App/AppServices.hpp>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Navigator.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

PrintSettingsDialog::PrintSettingsDialog(Biz::ProjectInteractor& project_interactor, Navigator& navigator) :
    AbstractSettingsDialog({}, "PrintSettingsDialog"),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_tool_cbi_list(project_interactor.preset_interactor().tool_cbi_list())
{
    Tab* tab = append_tab(_u8L("Print"));
    m_tabs.emplace_back(
        std::make_unique<PrintSettingsTab>(
            &project_interactor.preset_interactor().print_cbi(),
            tab,
            project_interactor
        )
    );

    Item* footer_items = m_footer->emplace_back<Item>();
    footer_items->set_gap(5.f);
    footer_items->emplace_back<LayoutButton>(_u8("Compare"), Render::Icon::Compare)->callbacks().action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(m_project_interactor.preset_interactor(), Domain::Preset::PresetKind::FdmPrint);
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
    for (size_t tab_index = 1; tab_index < m_tabs.size(); ++tab_index) {
        remove_tab(1);
    }
    m_tabs.erase(m_tabs.cbegin() + 1, m_tabs.cend());

    for (size_t tool_cbi_index = 0; tool_cbi_index < m_tool_cbi_list.size(); ++tool_cbi_index) {
        Biz::ConfigBoxInteractor& cbi = const_cast<Biz::ConfigBoxInteractor&>(
            m_tool_cbi_list.at(tool_cbi_index)
        );

        Tab* tab = append_tab(fmt::format("Tool {}", tool_cbi_index + 1));
        m_tabs.emplace_back(std::make_unique<PrintSettingsTab>(&cbi, tab, m_project_interactor));
    }
}

void PrintSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

PrintSettingsDialog::PrintSettingsTab::PrintSettingsTab(
    Biz::ConfigBoxInteractor* cbi,
    Yoga::AbstractSettingsDialog::Tab* tab,
    Biz::ProjectInteractor& project_interactor
) :
    cbi(cbi),
    tab(tab),
    observable_categorizer(std::make_shared<ObservableCategorizer>()),
    category_page_transformer(std::make_shared<CategoryPageTransformer>())
{
    observable_categorizer->set_source_model(cbi->config_box_list());
    category_page_transformer->set_project_interactor(&project_interactor);
    category_page_transformer->set_source_model(observable_categorizer.get());
    for (size_t i = 0; i < category_page_transformer->size(); ++i) {
        tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
            observable_categorizer->at(i).def().category,
            project_interactor.preset_interactor(),
            *cbi
        );
    }
    tab->page_list_view->set_source_list(category_page_transformer.get());
}

} // namespace Slic3r::App
