///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/App/ConfigRowItem.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {
class Text;
class Rectangle;
}

namespace Slic3r::Biz {
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigSubcategoryItem : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
    using ConfigRowListView = Yoga::ListView<ConfigRowItem, Domain::ConfigItem>;
public:
    ConfigSubcategoryItem(size_t index, const Domain::ConfigItem& data, Biz::ConfigBoxInteractor& cbi);

private:
    void on_data_update() override;

    void on_index_update() override;

private:
    Biz::ConfigBoxInteractor& m_cbi;

    ConfigRowListView* m_rows_list_view{nullptr};
    Biz::ObservableListSortFilter<Domain::ConfigItem> m_rows_filter_list;
    Yoga::Text* m_label{nullptr};
    Yoga::Rectangle* m_background{nullptr};
};

} // namespace Slic3r::App
