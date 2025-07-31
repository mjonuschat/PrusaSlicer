///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/FilamentSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

FilamentSettingsDialog::FilamentSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    AbstractSettingsDialog({}),
    m_project_interactor(project_interactor),
    m_material_cbi_list(m_project_interactor.preset_interactor().material_cbi_list())
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    m_material_cbi_list.add_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);

    on_reset();
}

FilamentSettingsDialog::~FilamentSettingsDialog()
{
    m_material_cbi_list.remove_listener<Biz::IListObserver<Biz::ConfigBoxInteractor>>(this);
}

void FilamentSettingsDialog::on_reset()
{
    for (size_t tab_index = 0; tab_index < m_filaments.size(); ++tab_index) {
        remove_tab(0);
    }
    m_filaments.clear();

    for (size_t material_cbi_index = 0; material_cbi_index < m_material_cbi_list.size();
         ++material_cbi_index)
    {
        Biz::ConfigBoxInteractor& cbi = const_cast<Biz::ConfigBoxInteractor&>(
            m_material_cbi_list.at(material_cbi_index)
        );

        Tab* tab = append_tab(fmt::format("Filament {}", material_cbi_index + 1));
        m_filaments.emplace_back(
            std::make_unique<FilamentTab>(&cbi, tab, m_project_interactor.preset_interactor())
        );
    }
}

FilamentSettingsDialog::FilamentTab::FilamentTab(
    Biz::ConfigBoxInteractor* cbi,
    Tab* tab,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    cbi(cbi),
    tab(tab)
{
    observable_categorizer.set_source_model(cbi->config_box_list());
    category_page_transformer.set_source_model(&observable_categorizer);
    for (size_t i = 0; i < category_page_transformer.size(); ++i) {
        tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
            observable_categorizer.at(i).def().category,
            preset_interactor,
            *cbi
        );
    }
    tab->page_list_view->set_source_list(&category_page_transformer);
}

} // namespace Slic3r::App
