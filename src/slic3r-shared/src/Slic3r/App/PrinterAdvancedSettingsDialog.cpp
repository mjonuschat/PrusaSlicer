///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/ConfigSubcategoryListView.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PrinterAdvancedSettingsDialog::PrinterAdvancedSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    AbstractSettingsDialog({"Printer"}),
    m_project_interactor(project_interactor),
    m_cbi(project_interactor.preset_interactor().printer_cbi())
{
    m_category_page_transformer.set_source_model(&m_observable_categorizer);
    m_observable_categorizer.set_source_model(&m_cbi.config_box_list());

    m_current_tab->page_list_view->set_source_list(&m_category_page_transformer);

    content_item()->set_width(650);
    content_item()->set_height(700);

    // m_pages_stack_layout->set_orientation(Orientation::Vertical);
    for (size_t i = 0; i < m_category_page_transformer.size(); ++i) {
        m_current_tab->pages_stack_layout->emplace_back<ConfigSubcategoryListView>(
            m_observable_categorizer.at(i).def().category,
            m_cbi
        );
    }

    // dynamic_cast<PageEntryButton*>(m_page_list_view->get_item(0))->set_checked(true);
}

} // namespace Slic3r::App
