///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ConfigSubcategoryItem.hpp"

#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigSubcategoryItem::ConfigSubcategoryItem(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    Biz::ConfigBoxInteractor& cbi
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_cbi(cbi),
    m_preset_interactor(preset_interactor),
    m_rows_filter_list(std::make_shared<Biz::ObservableListSortFilter<Domain::ConfigItem>>())
{
    set_orientation(Orientation::Vertical);
    set_flex_shrink(0);

    m_background = emplace_back<Rectangle>();
    m_background->set_flags(ImDrawFlags_None);
    m_background->set_rounding(0);
    m_background->set_orientation(Orientation::Vertical);
    m_background->set_gap(10);
    m_background->set_padding(10);
    m_background->set_flex_shrink(0);

    m_label = m_background->emplace_back<Text>(m_state->def().option_group, Render::ImguiFontType::Bold);

    m_rows_filter_list->set_source_model(m_cbi.config_box_list());
    const std::string option_group = m_state->def().option_group; // Intentional copy
    const Domain::ConfigItemDef::Category category = m_state->def().category; // Intentional copy
    m_rows_filter_list->set_filter_fn([option_group, category](const Domain::ConfigItem& item) -> bool {
        return item.def().option_group == option_group && item.def().category == category;
    });

    m_rows_list_view = m_background->emplace_back<ConfigRowListView>(m_preset_interactor);
    m_rows_list_view->set_source_list(m_rows_filter_list.get());
    m_rows_list_view->set_orientation(Orientation::Vertical);
    m_rows_list_view->set_gap(5);
    m_rows_list_view->set_padding(10);

    on_index_update();
}

void ConfigSubcategoryItem::on_data_update()
{
    m_label->set_text(m_state->def().option_group);
}

void ConfigSubcategoryItem::on_index_update()
{
    m_background->set_fill(m_index % 2 == 0 ? ImColor(27, 27, 27) : ImColor(22, 22, 22));
}

} // namespace Slic3r::App
