///|/ Copyright (c) Prusa Research 2026 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/ConfigDef.hpp>

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/PrintToolItem.hpp"

#include "Slic3r/App/Config/PrintToolRowItem.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz {
class PrintToolConfigBoxInteractor;
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class PrintToolFavoritesItem : public Yoga::Item
{
    using PrintToolRowListViewFactory = Yoga::ViewFactory<
        PrintToolRowItem,
        Biz::PrintToolItem,
        Biz::PrintToolConfigBoxInteractor&,
        Biz::IConfigBoxSetter&>;
    using PrintToolRowListView =
        Yoga::ListView<PrintToolRowItem, Biz::PrintToolItem, PrintToolRowListViewFactory>;

public:
    PrintToolFavoritesItem(
        Biz::PrintToolConfigBoxInteractor& cbi,
        Biz::IConfigBoxSetter& cbi_setter
    );

private:
    Biz::PrintToolConfigBoxInteractor& m_cbi;
    Biz::IConfigBoxSetter& m_cbi_setter;

    PrintToolRowListView* m_rows_list_view{nullptr};
    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Biz::PrintToolItem>> m_rows_filter_list;
};

} // namespace Slic3r::App
