///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/OverrideOptionGroup.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

OverrideOptionGroup::OverrideOptionGroup(
    size_t index,
    const Biz::OverrideItem& data,
    Biz::ProjectInteractor& project_interactor
) :
    Biz::DataObserver<Biz::OverrideItem>(index, data),
    m_project_interactor(project_interactor)
{
    set_orientation(Orientation::Vertical);
    set_gap(5);

    Item* label_row = emplace_back<Item>();
    label_row->set_gap(5);
    m_text_group_name = label_row->emplace_back<Text>(std::string());
    m_text_group_name->set_font_type(Render::ImguiFontType::Bold);
    m_text_group_name->set_flex_grow(1);

    LayoutButton* remove_group_button = label_row->emplace_back<LayoutButton>(
        std::string(),
        Render::Icon::Minus,
        Biz::_u8L("Remove override group")
    );
    remove_group_button->callbacks().action = [this]
    {
        for (int index = m_override_config_filter->size() - 1; index >= 0; --index) {
            m_project_interactor.preset_interactor().set_item_override(
                *m_override_config_filter->at(index).config_item,
                false
            );
        }
    };

    m_override_config_filter = std::make_shared<OverrideConfigFilter>();
    m_override_config_filter->set_filter_fn(
        [this](const Biz::OverrideItem& item) -> bool
        {
            return item.is_override()
                && item.config_item->def().category == m_category
                && item.overriden.value();
        }
    );

    m_override_config_list_view = emplace_back<OverrideConfigListView>(
        OverrideConfigListViewFactory{m_project_interactor.preset_interactor(), true}
    );
    m_override_config_list_view->set_orientation(Orientation::Vertical);
    m_override_config_list_view->set_gap(5);
    m_override_config_list_view->set_source_list(m_override_config_filter.get());

    m_override_config_filter->set_source_model(m_project_interactor.preset_interactor()
                                                   .object_settings_interactor()
                                                   .object_observable_list());

    m_separator = emplace_back<Separator>(Orientation::Horizontal);

    on_data_update();
}

void OverrideOptionGroup::on_data_update()
{
    m_text_group_name->set_text(
        Domain::ConfigItemDef::translate_category(
            m_state->config_item->def().category,
            m_project_interactor.selected_config_container().print_technology()
        )
    );

    const Domain::ConfigItemDef::Category category = m_state->config_item->def().category;
    if (m_category != category) {
        m_category = category;
        m_override_config_filter->invalidate();
    }
}

} // namespace Slic3r::App
