///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrintSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

PrintSettingsDialog::PrintSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    AbstractSettingsDialog({}),
    m_project_interactor(project_interactor)
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    PrintSettingsTab print_tab{project_interactor.preset_interactor().print_cbi()};
    print_tab.tab = append_tab("Print");
    print_tab.observable_categorizer.set_source_model(&print_tab.cbi.config_box_list());
    print_tab.category_page_transformer.set_source_model(&print_tab.observable_categorizer);
    for (size_t i = 0; i < print_tab.category_page_transformer.size(); ++i) {
        print_tab.tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
            print_tab.observable_categorizer.at(i).def().category,
            print_tab.cbi
        );
    }
    print_tab.tab->page_list_view->set_source_list(&print_tab.category_page_transformer);
    m_tabs.push_back(std::move(print_tab));

    Biz::BatchObservableList<Biz::ConfigBoxInteractor>& tool_cbi_list = project_interactor
                                                                       .preset_interactor()
                                                                       .tool_cbi_list();
    for (size_t tool_cbi_index = 0; tool_cbi_index < tool_cbi_list.size(); ++tool_cbi_index) {
        Biz::ConfigBoxInteractor& cbi = const_cast<Biz::ConfigBoxInteractor&>(
            tool_cbi_list.at(tool_cbi_index)
        );

        PrintSettingsTab tab{cbi};
        tab.tab = append_tab(fmt::format("Filament {}", tool_cbi_index + 1));
        tab.observable_categorizer.set_source_model(&cbi.config_box_list());
        tab.category_page_transformer.set_source_model(&tab.observable_categorizer);

        for (size_t i = 0; i < tab.category_page_transformer.size(); ++i) {
            tab.tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
                tab.observable_categorizer.at(i).def().category,
                tab.cbi
            );
        }

        tab.tab->page_list_view->set_source_list(&tab.category_page_transformer);

        m_tabs.push_back(std::move(tab));
    }

    m_footer->emplace_back<LayoutButton>("Save preset");
}

} // namespace Slic3r::App
