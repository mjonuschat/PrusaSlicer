///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ConfigSubcategoryListView.hpp"

#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigSubcategoryListView::ConfigSubcategoryListView(
    Domain::ConfigItemDef::Category category,
    Biz::Preset::PresetInteractor& preset_interactor,
    Biz::ConfigBoxInteractor& cbi
) :
    m_preset_interactor(preset_interactor),
    m_cbi(cbi),
    m_category_filter(std::make_shared<Biz::ObservableListSortFilter<Domain::ConfigItem>>())
{
    set_orientation(Orientation::Vertical);
    set_gap(5);
    set_flex_grow(1);
    set_min_size({0, 100});

    Text* category_label = emplace_back<Text>(
        Domain::ConfigItemDef::translate_category(category),
        Render::ImguiFontType::Bold
    );
    category_label->set_margin(10);

    m_category_filter->set_filter_fn([=](const Domain::ConfigItem& config_item) {
        return config_item.def().category == category;
    });

    m_category_filter->set_group_by_fn([](const Domain::ConfigItem& config_item,
                                          std::unordered_set<std::string>& seen_keys) {
        if (seen_keys.contains(config_item.def().option_group)) {
            return true;
        } else {
            seen_keys.insert(config_item.def().option_group);
            return false;
        }
    });

    m_category_filter->set_source_model(m_cbi.config_box_list());

    m_list_view = emplace_back<SubcategoryListView>(
        SubcategoryListViewFactory{m_preset_interactor, m_cbi}
    );
    m_list_view->set_orientation(Orientation::Vertical);
    m_list_view->set_source_list(m_category_filter.get());
    m_list_view->set_gap(5);
}

ConfigSubcategoryListView::~ConfigSubcategoryListView()
{
    m_list_view->set_source_list(nullptr);
}

} // namespace Slic3r::App
