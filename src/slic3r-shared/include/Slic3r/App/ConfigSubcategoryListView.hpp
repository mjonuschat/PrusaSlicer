///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ConfigSubcategoryItem.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/Biz/ObservableListSortFilter.hpp"

namespace Slic3r::Biz {
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class Rectangle;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

using ConfigSubcategoryListViewFactory = Yoga::ViewFactory<
    ConfigSubcategoryItem,
    Domain::ConfigItem,
    Biz::Preset::PresetInteractor&,
    Biz::ConfigBoxInteractor&>;

class ConfigSubcategoryListView :
    public Yoga::ListView<
        ConfigSubcategoryItem,
        Domain::ConfigItem,
        ConfigSubcategoryListViewFactory,
        Yoga::ScrollArea>
{
public:
    explicit ConfigSubcategoryListView(
        Domain::ConfigItemDef::Category category,
        Biz::Preset::PresetInteractor& preset_interactor,
        Biz::ConfigBoxInteractor& cbi
    );
    ~ConfigSubcategoryListView();

    void navigate_to_item(const Domain::ConfigItem* config_item);
    void clear_navigation();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    Biz::ConfigBoxInteractor& m_cbi;
    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Domain::ConfigItem>> m_category_filter;
    Yoga::Rectangle* m_background{nullptr};
};

} // namespace Slic3r::App
