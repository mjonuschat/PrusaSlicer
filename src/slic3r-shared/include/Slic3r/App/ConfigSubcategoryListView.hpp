///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/App/ConfigSubcategoryItem.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/Biz/ObservableListSortFilter.hpp"

namespace Slic3r::App::Yoga {
class Rectangle;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigSubcategoryListView : public Yoga::ScrollArea
{
    using SubcategoryListView = Yoga::ListView<
        ConfigSubcategoryItem,
        Domain::ConfigItem,
        Yoga::ViewFactory<ConfigSubcategoryItem, Domain::ConfigItem, Biz::ConfigBoxInteractor&>>;

public:
    explicit ConfigSubcategoryListView(
        Domain::ConfigItemDef::Category category,
        Biz::ConfigBoxInteractor& cbi
    );

private:
    Biz::ConfigBoxInteractor& m_cbi;
    SubcategoryListView* m_list_view{nullptr};
    Biz::ObservableListSortFilter<Domain::ConfigItem> m_category_filter;
    Yoga::Rectangle* m_background{nullptr};
};

} // namespace Slic3r::App
