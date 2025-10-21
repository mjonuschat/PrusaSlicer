#include "Slic3r/App/ObjectListWindow.hpp"
#include "Slic3r/App/ObjectList.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/SLAResultCache.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"

#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "libslic3r/format.hpp"
#include "libslic3r/Utils.hpp"

#include <boost/nowide/convert.hpp>
#include <boost/nowide/iostream.hpp>

using namespace Slic3r::Biz;

namespace Slic3r::App {

using namespace Yoga;

ObjectListWindow::ObjectListWindow(Biz::ProjectInteractor* project_interactor, bool for_plater)
    : Window("object_list"),
    m_project_interactor(project_interactor)
{
    set_orientation(Orientation::Vertical);
    set_min_size({330.f, 0.f});
    set_padding(0.f);
    set_gap(0.f);

    Rectangle* main_bg = emplace_back<Rectangle>();
    main_bg->set_orientation(Orientation::Vertical);
    main_bg->set_fill(IM_COL32_BLACK_TRANS);
    main_bg->set_flex_grow(1.f);
    main_bg->set_rounding(3.f);
    main_bg->set_padding(Paddings(0.f, 15.f));

    Item* header = main_bg->emplace_back<Item>();
    header->set_gap(10.f);
    header->set_padding(Paddings(15.f, 0.f));

    m_label = header->emplace_back<Text>(_u8L("Object List"));
    m_label->set_font_type(App::Render::ImguiFontType::Bold);
    m_label->set_flex_grow(1.f);

    const ObjectList::Mode mode = for_plater ? ObjectList::Mode::Plater : ObjectList::Mode::Preview;
    if (mode == ObjectList::Mode::Plater) {
        m_add_container_button = main_bg->emplace_back<Yoga::LayoutButton>(
            _u8L("Add Printer Container"),
            Render::Icon::ConfigContainer
        );
        m_add_container_button->set_background_color(ImColor(41, 41, 41));
        m_add_container_button->callbacks().action = [this]()
        {
            m_project_interactor->add_config_container();
            if (on_config_container_added != nullptr)
                on_config_container_added();
        };
    }

    m_object_list = main_bg->emplace_back<ObjectList>(project_interactor, mode);
    m_object_list->set_flex_grow(1.f);

    if (mode == ObjectList::Mode::Plater) {
        LayoutButton* show_details_button = header->emplace_back<LayoutButton>("", Render::Icon::Details, _u8L("Show item details"));
        show_details_button->set_checkable(true);
        show_details_button->callbacks().checked_changed = [this](bool checked) {
            m_object_list->selected_project_context().show_details = checked;            
        };
    }
    else {
        LayoutButton* scene_map_button = header->emplace_back<LayoutButton>("", Render::Icon::SceneMap, _u8L("Show scene map"));
        scene_map_button->set_checkable(true);
        scene_map_button->callbacks().checked_changed = [this, main_bg, header](bool checked) {
            m_object_list->set_visible(!checked);
            m_scene_map->set_visible(checked);
            m_material_cost_row->set_visible(checked);
            m_first_layer_time_row->set_visible(checked);

            m_label->set_text(checked ? _u8L("Scene Map") : _u8L("Object List"));
            main_bg->set_margin(checked ? 5.f : 0.f);
            main_bg->set_padding(checked ? Paddings(10.f, 10.f) : Paddings(0.f, 15.f));
            header->set_padding(Paddings(checked ? 0.f : 15.f, 0.f));
            if (checked)
                main_bg->set_fill(ImColor(84, 84, 84));
            else
                main_bg->set_fill(IM_COL32_BLACK_TRANS);

            m_object_list->selected_project_context().scene_map = checked;
        };

        scene_map_button->set_visible(false);// ysFIXME = Delete this line, when implementation of the Scene map will be completed

        m_scene_map = main_bg->emplace_back<Item>();
        m_scene_map->set_orientation(Orientation::Vertical);
        m_scene_map->set_visible(false);
        m_scene_map->set_flex_grow(1.f);

        Item* map = m_scene_map->emplace_back<Item>();
        map->set_flex_grow(1.f);
        Item* scene_actions = m_scene_map->emplace_back<Item>();
        scene_actions->set_gap(5.f);

        ToggleButton* isometric_view = scene_actions->emplace_back<ToggleButton>(_u8L("Isometric view"));
        isometric_view->set_flex_grow(1.f);
        isometric_view->set_font_type(Render::ImguiFontType::Bold);

        LayoutButton* add_btn = scene_actions->emplace_back<LayoutButton>("", Render::Icon::TobBarPlus);
        add_btn->set_background_color(ImColor(37, 37, 37));
        LayoutButton* del_btn = scene_actions->emplace_back<LayoutButton>("", Render::Icon::TopBarCross);
        del_btn->set_background_color(ImColor(37, 37, 37));

        m_sliced_info = emplace_back<Rectangle>();
        m_sliced_info->set_orientation(Orientation::Vertical);
        m_sliced_info->set_fill(ImColor(32, 32, 32));
        m_sliced_info->set_flags(ImDrawFlags_RoundCornersBottom);
        m_sliced_info->set_padding(15.f);
        m_sliced_info->set_gap(10.f);
        m_sliced_info->set_visible(false);

        Item* si_header = m_sliced_info->emplace_back<Item>();
        Text* si_label = si_header->emplace_back<Text>(_u8L("Sliced Info"));
        si_label->set_font_type(Render::ImguiFontType::Bold);
        si_label->set_flex_grow(1.f);
        LayoutButton* calc_btn = si_header->emplace_back<LayoutButton>("", Render::Icon::Calculator);

        auto add_row = [this](const std::string& label, Text** value_item) {
            Item* row = m_sliced_info->emplace_back<Item>();
            row->set_gap(10.f);

            Text* text = row->emplace_back<Text>(label);
            text->set_font_type(Render::ImguiFontType::Bold);
            text->set_width(85.f);

            *value_item = row->emplace_back<Text>("");
            return row;
        };

        Item* used_material_row = add_row(_u8L("Used material"), &m_used_material);
        m_material_cost_row = add_row(_u8L("Cost"), &m_material_cost);
        m_material_cost_row->set_visible(false);
        m_first_layer_time_row = add_row(_u8L("First layer"), &m_first_layer_time);
        m_first_layer_time_row->set_visible(false);
        Item* estimated_time_row = add_row(_u8L("Printing time"), &m_estimated_time);
    }
}

void ObjectListWindow::update_sliced_info()
{
    const Domain::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status{
        m_project_interactor->status_cache().get_status(id) };

    const bool is_finished = status && status->code == Biz::Slicing::StatusCode::Finished;
    m_sliced_info->set_visible(is_finished);

    if (!is_finished) {
        return;
    }

    const std::optional<Biz::FDMResultRef> fdm_result{ m_project_interactor->fdm_result_cache().get_result(id) };
    if (fdm_result) {
        const Biz::libpgcode::PrintEstimatedStatistics& print_statistics = fdm_result->get().print_statistics;

        float volume{ 0.f };
        for (const auto& [_, vol] : print_statistics.volumes_per_extruder)
            volume += vol;

        const double filament_density = 1.25e-3f; // g/mm^3  ; Common filaments are very lightweight, so precise number is not that important
        float weight = volume * filament_density;

        float length{ 0.f };
        for (const auto& [_, len] : print_statistics.used_filaments_per_role)
            length += len.first;

        const std::string used_material = format("%1$.2f g  %2$.2f m  %3$.0f mm3", weight, length, volume);
        m_used_material->set_text(used_material);

        float cost{ 0.f };
        for (const auto& [_, c] : print_statistics.cost_per_extruder)
            cost += c;
        m_material_cost->set_text(format("%1%", cost));

        const std::string first_layer_time = "? seconds";
        m_first_layer_time->set_text(first_layer_time);

        const std::string estimated_time = Slic3r::get_time_dhms(print_statistics.modes[size_t(Biz::libpgcode::TimeMode::Normal)].time);
        m_estimated_time->set_text(estimated_time);

        return;
    }

    const std::optional<Biz::SLAResultRef> sla_result{ m_project_interactor->sla_result_cache().get_result(id) };
    if (!sla_result || !sla_result->get().print_statistics)
        return;

    const Biz::Slicing::Sla::PrintStatistics& print_statistics = *sla_result->get().print_statistics;

    float used_material_total = print_statistics.objects_used_material + print_statistics.support_used_material;

    const std::string used_material = format("%1$.2f g  %2$.0f mm3", print_statistics.total_weight, used_material_total);
    m_used_material->set_text(used_material);

    m_material_cost->set_text(format("%1%", print_statistics.total_cost));

    m_first_layer_time->set_text(format("%1% seconds", print_statistics.layers_times_running_total[0]));

    const std::string estimated_time = Slic3r::get_time_dhms(print_statistics.estimated_print_time);
    m_estimated_time->set_text(estimated_time);
}

void ObjectListWindow::set_bed_instance_icons(const Plater::BedThumbnailTextures& icons)
{
    m_object_list->set_bed_instance_icons(icons);
}

} // namespace Slic3r::App
