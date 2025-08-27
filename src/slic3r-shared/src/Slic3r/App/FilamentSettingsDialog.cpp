///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/FilamentSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

FilamentSettingsDialog::FilamentSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    AbstractSettingsDialog({}, "FilamentSettingsDialog"),
    m_project_interactor(project_interactor),
    m_material_cbi_list(m_project_interactor.preset_interactor().material_cbi_list())
{
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

        std::string tab_name = m_project_interactor.selected_config_container().print_technology() == Domain::PrinterTechnology::FFF ? _u8L("Filament") : _u8L("Material");

        Tab* tab = append_tab(fmt::format("{} {}", tab_name, material_cbi_index + 1));
        m_filaments.emplace_back(
            std::make_unique<FilamentTab>(&cbi, tab, m_project_interactor)
        );
    }
}

FilamentSettingsDialog::FilamentTab::FilamentTab(
    Biz::ConfigBoxInteractor* cbi,
    Tab* tab,
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
