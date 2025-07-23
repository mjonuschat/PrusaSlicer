///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/FilamentSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

FilamentSettingsDialog::FilamentSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    AbstractSettingsDialog({}),
    m_project_interactor(project_interactor)
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    Biz::BatchObservableList<Biz::ConfigBoxInteractor>& material_cbi_list = project_interactor
                                                                           .preset_interactor()
                                                                           .material_cbi_list();
    for (size_t material_cbi_index = 0; material_cbi_index < material_cbi_list.size();
         ++material_cbi_index)
    {
        Biz::ConfigBoxInteractor& cbi = const_cast<Biz::ConfigBoxInteractor&>(
            material_cbi_list.at(material_cbi_index)
        );

        FilamentTab tab{cbi};
        tab.tab = append_tab(fmt::format("Filament {}", material_cbi_index + 1));
        tab.observable_categorizer.set_source_model(&cbi.config_box_list());
        tab.category_page_transformer.set_source_model(&tab.observable_categorizer);

        for (size_t i = 0; i < tab.category_page_transformer.size(); ++i) {
            tab.tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
                tab.observable_categorizer.at(i).def().category,
                tab.cbi
            );
        }

        tab.tab->page_list_view->set_source_list(&tab.category_page_transformer);

        m_filaments.push_back(tab);
    }
}

} // namespace Slic3r::App
