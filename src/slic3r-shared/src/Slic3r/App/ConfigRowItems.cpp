///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ConfigRowItems.hpp"

#include "Slic3r/App/Yoga/Text.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

Slic3r::App::ConfigRowItems::ConfigRowItems(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    Biz::ConfigBoxInteractor& cbi
) :
    DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor),
    m_cbi(cbi)
{
    if (m_state->def().row_group.empty()) {
        m_single_item = emplace_back<ConfigRowItem>(index, data, m_preset_interactor, false);
        m_single_item->set_flex_grow(1);
    } else {
        m_label = emplace_back<Text>(m_state->def().row_group);
        m_label->set_width(150);
        m_label->set_self_align(YGAlignCenter);

        m_row_items_filter = std::make_shared<Biz::ObservableListSortFilter<Domain::ConfigItem>>();
        const std::string option_group = m_state->def().option_group; // Intentional copy
        const std::string row_group    = m_state->def().row_group; // intentional copy
        const Domain::ConfigItemDef::Category category =
            m_state->def().category; // Intentional copy

        m_row_items_filter->set_filter_fn(
            [option_group, category, row_group](const Domain::ConfigItem& item) -> bool
            {
                return item.def().option_group == option_group
                    && item.def().category == category
                    && item.def().row_group == row_group;
            }
        );
        m_row_items_filter->set_source_model(m_cbi.config_box_list());

        m_row_group_list_view =
            emplace_back<ConfigRowListView>(ConfigRowListViewFactory{m_preset_interactor, true});
        m_row_group_list_view->set_align_items(YGAlignCenter);
        m_row_group_list_view->set_gap(5);
        m_row_group_list_view->set_flex_grow(1);
        m_row_group_list_view->set_source_list(m_row_items_filter.get());
    }
}

void ConfigRowItems::navigate_to_item(const Domain::ConfigItem* config_item)
{
    if (m_single_item) {
        m_single_item->navigate_to_item(config_item);
    } else {
        for (size_t column_index = 0; column_index < m_row_items_filter->size(); ++column_index) {
            m_row_group_list_view->item_at(column_index)->navigate_to_item(config_item);
        }
    }
}

void ConfigRowItems::clear_navigation()
{
    if (m_single_item) {
        m_single_item->clear_navigation();
    } else {
        for (size_t column_index = 0; column_index < m_row_items_filter->size(); ++column_index) {
            m_row_group_list_view->item_at(column_index)->clear_navigation();
        }
    }
}

void Slic3r::App::ConfigRowItems::on_data_update()
{
    if (m_single_item) {
        m_single_item->set_state(*m_state);
    }
}

} // namespace Slic3r::App
