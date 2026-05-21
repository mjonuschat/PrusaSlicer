///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigSubcategoryListView.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigSubcategoryListView::ConfigSubcategoryListView(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    Biz::ConfigBoxInteractor& cbi,
    size_t cbi_index
) :
    ListView<
        ConfigSubcategoryItem,
        Domain::ConfigItem,
        ConfigSubcategoryListViewFactory,
        ScrollArea>(ConfigSubcategoryListViewFactory{cbi_container, cbi, cbi_index}),
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_cbi_container(cbi_container),
    m_cbi(cbi),
    m_cbi_index(cbi_index),
    m_category_filter(
        std::make_shared<
            Biz::ObservableListSortFilter<Domain::ConfigItem, Domain::ConfigItemDef::OptionGroup>>()
    )
{
    set_object_name("ConfigSubcategoryListView");
    set_orientation(Orientation::Vertical);
    set_flex_grow(1);
    set_gap(0);
    set_flex_grow(1);
    set_min_height(100);

    m_category_filter->set_filter_fn([this](const Domain::ConfigItem& config_item)
                                     { return config_item.def().category == m_category; });
    m_category_filter->set_group_by_fn(
        [](const Domain::ConfigItem& config_item,
           std::unordered_set<Domain::ConfigItemDef::OptionGroup>& seen_keys)
        {
            if (seen_keys.contains(config_item.def().option_group)) {
                return true;
            } else {
                seen_keys.insert(config_item.def().option_group);
                return false;
            }
        }
    );

    m_category_filter->set_sort_fn([](const Domain::ConfigItem& lhs, const Domain::ConfigItem& rhs)
                                   { return lhs.def().option_group < rhs.def().option_group; });

    m_category_filter->set_source_model(m_cbi.config_box_list());

    set_source_list(m_category_filter.get());

    on_data_update();
}

void ConfigSubcategoryListView::navigate_to_item(const Domain::ConfigItem* config_item)
{
    const Domain::ConfigItemDef::OptionGroup option_group = config_item->def().option_group;
    for (size_t subcategory_index = 0; subcategory_index < m_category_filter->size();
         ++subcategory_index)
    {
        if (m_category_filter->at(subcategory_index).def().option_group == option_group) {
            ConfigSubcategoryItem* found_item = item_at(subcategory_index);
            found_item->navigate_to_item(config_item);
            scroll_at_item(found_item);
            break;
        }
    }
}

void ConfigSubcategoryListView::clear_navigation()
{
    for (size_t subcategory_index = 0; subcategory_index < m_category_filter->size();
         ++subcategory_index)
    {
        item_at(subcategory_index)->clear_navigation();
    }
}

void ConfigSubcategoryListView::on_data_update()
{
    Domain::ConfigItemDef::Category category = m_state->def().category;
    if (m_category != category) {
        m_category = category;
        m_category_filter->invalidate();
    }
}

} // namespace Slic3r::App
