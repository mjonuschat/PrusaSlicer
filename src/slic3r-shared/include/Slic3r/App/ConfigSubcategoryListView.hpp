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

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Rectangle;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

using ConfigSubcategoryListViewFactory = Yoga::ViewFactory<
    ConfigSubcategoryItem,
    Domain::ConfigItem,
    Biz::IConfigBoxSetter&,
    Biz::ConfigBoxInteractor&,
    size_t>;

class ConfigSubcategoryListView :
    public Yoga::ListView<
        ConfigSubcategoryItem,
        Domain::ConfigItem,
        ConfigSubcategoryListViewFactory,
        Yoga::ScrollArea>,
    public Biz::DataObserver<Domain::ConfigItem>
{
public:
    explicit ConfigSubcategoryListView(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        Biz::ConfigBoxInteractor& cbi,
        size_t cbi_index
    );

    void navigate_to_item(const Domain::ConfigItem* config_item);
    void clear_navigation();

protected:
    void on_data_update() override;

private:
    Biz::IConfigBoxSetter& m_cbi_container;
    Biz::ConfigBoxInteractor& m_cbi;
    size_t m_cbi_index{0};

    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Domain::ConfigItem>> m_category_filter;
    Yoga::Rectangle* m_background{nullptr};
    Domain::ConfigItemDef::Category m_category{Domain::ConfigItemDef::Category::Unkown};
};

} // namespace Slic3r::App
