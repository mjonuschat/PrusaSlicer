///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/DataObserver.hpp"

#include "Slic3r/App/ConfigRowItems.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::Biz {
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigSubcategoryItem : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Rectangle
{
    using ConfigRowListViewFactory = Yoga::ViewFactory<
        ConfigRowItems,
        Domain::ConfigItem,
        Biz::IConfigBoxSetter&,
        Biz::ConfigBoxInteractor&,
        size_t>;
    using ConfigRowListView =
        Yoga::ListView<ConfigRowItems, Domain::ConfigItem, ConfigRowListViewFactory>;

public:
    ConfigSubcategoryItem(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        Biz::ConfigBoxInteractor& cbi,
        size_t cbi_index
    );

    void navigate_to_item(const Domain::ConfigItem* config_item);
    void clear_navigation();

private:
    void on_data_update() override;

    void on_index_update() override;

private:
    Biz::ConfigBoxInteractor& m_cbi;
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};

    ConfigRowListView* m_rows_list_view{nullptr};
    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Domain::ConfigItem>> m_rows_filter_list;
    Yoga::Text* m_label{nullptr};
    std::string m_option_group;
    Domain::ConfigItemDef::Category m_category{Domain::ConfigItemDef::Category::Unkown};
};

} // namespace Slic3r::App
