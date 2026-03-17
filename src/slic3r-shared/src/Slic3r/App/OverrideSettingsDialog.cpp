///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/OverrideSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

OverrideSettingsDialog::OverrideSettingsDialog(Biz::ProjectInteractor& project_interactor) :
    Dialog({Biz::_u8L("Settings")}, "OverrideSettingsDialog"),
    m_project_interactor(project_interactor)
{
    content()->set_width(380);

    Biz::Preset::PresetInteractor& preset_interactor = project_interactor.preset_interactor();

    content()->set_orientation(Orientation::Vertical);
    m_stack_layout = content()->emplace_back<StackLayout>();
    m_stack_layout->set_orientation(Orientation::Vertical);

    m_categorizer = std::make_shared<ObservableOverrideCategorizer>();

    m_select_category = [this](Domain::ConfigItemDef::Category category)
    {
        if (m_current_category != category) {
            m_current_category = category;
            m_category_filter->invalidate();
            m_options_category_text->set_text(
                Biz::_u8(
                    Domain::ConfigItemDef::translate_category(
                        category,
                        m_project_interactor.selected_config_container().print_technology()
                    )
                )
            );
        }
        m_stack_layout->set_current_index(1);
    };

    m_override_category_list_view = m_stack_layout->emplace_back<OverrideCategoryListView>(
        OverrideCategoryFactory{project_interactor, m_select_category}
    );
    m_override_category_list_view->set_orientation(Orientation::Vertical);
    m_override_category_list_view->set_gap(5);
    m_override_category_list_view->set_padding(0);
    m_override_category_list_view->set_source_list(m_categorizer.get());

    m_categorizer->set_source_model(
        preset_interactor.object_settings_interactor().object_observable_list()
    );

    Item* options_page = m_stack_layout->emplace_back<Item>();
    options_page->set_orientation(Orientation::Vertical);
    options_page->set_gap(5);

    Item* back_row = options_page->emplace_back<Item>();
    back_row->set_gap(5);
    back_row->set_flex_shrink(0);
    LayoutButton* back_button = back_row->emplace_back<LayoutButton>("", Render::Icon::ChevronLeft);
    back_button->set_width(18);
    back_button->set_height(18);
    back_button->set_content_padding(Paddings(2));
    back_button->callbacks().action = [this]() {
        on_about_to_show();
    };
    m_options_category_text = back_row->emplace_back<Text>(std::string());
    m_options_category_text->set_flex_grow(1);
    m_options_category_text->set_font_type(Render::ImguiFontType::Bold);

    options_page->emplace_back<Separator>(Orientation::Horizontal);

    m_category_filter = std::make_shared<OverrideConfigFilter>();
    m_category_filter->set_filter_fn([this](const Biz::OverrideItem& item) -> bool {
        return item.is_override() && item.config_item->def().category == m_current_category;
    });

    m_override_config_list_view = options_page->emplace_back<OverrideConfigListView>(
        OverrideConfigListViewFactory{preset_interactor, false}
    );
    m_override_config_list_view->set_orientation(Orientation::Vertical);
    m_override_config_list_view->set_gap(5);
    m_override_config_list_view->set_margin(Margins(0, 0, -10, 0));
    m_override_config_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_override_config_list_view->set_source_list(m_category_filter.get());
    m_override_config_list_view->set_max_size({YGUndefined, 350});

    m_category_filter->set_source_model(
        preset_interactor.object_settings_interactor().object_observable_list()
    );
}

void OverrideSettingsDialog::on_about_to_show()
{
    m_stack_layout->set_current_index(0);
}

} // namespace Slic3r::App
