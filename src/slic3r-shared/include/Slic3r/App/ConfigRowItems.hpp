///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/ConfigRowItem.hpp"
#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemSpinBox;

class ConfigRowItems : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
    using ConfigRowListViewFactory =
        Yoga::ViewFactory<ConfigRowItem, Domain::ConfigItem, Biz::Preset::PresetInteractor&, bool>;
    using ConfigRowListView =
        Yoga::ListView<ConfigRowItem, Domain::ConfigItem, ConfigRowListViewFactory>;

public:
    ConfigRowItems(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor,
        Biz::ConfigBoxInteractor& cbi
    );

    void navigate_to_item(const Domain::ConfigItem* config_item);
    void clear_navigation();

private:
    void on_data_update() override;

private:
    enum class InitializedType
    {
        None,
        Single,
        Multiple
    };
    InitializedType m_initialized_type{InitializedType::None};

    Biz::Preset::PresetInteractor& m_preset_interactor;
    Biz::ConfigBoxInteractor& m_cbi;

    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Domain::ConfigItem>> m_row_items_filter;
    ConfigRowListView* m_row_group_list_view{nullptr};
    ConfigRowItem* m_single_item{nullptr};
    Yoga::Text* m_label{nullptr};

    std::string m_option_group;
    std::string m_row_group;
    Domain::ConfigItemDef::Category m_category{Domain::ConfigItemDef::Category::Unkown};
};

} // namespace Slic3r::App
