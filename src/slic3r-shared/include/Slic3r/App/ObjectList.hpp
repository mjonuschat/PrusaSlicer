#pragma once

#include "MultiSelections.hpp"

#include <Slic3r/App/Yoga/Window.hpp>
#include <set>
#include <string>

namespace Slic3r {
class Model;
class ModelObject;
class ModelInstance;
class ModelVolume;
}

namespace Slic3r::Domain {
struct ElementRef;
struct BedInstance;
}

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::Biz::Scene {
class SceneInteractor;
struct Selection;
}

namespace Slic3r::App::Render {
class ImguiRender;
} // namespace Slic3r::App::Render

namespace Slic3r::App {

class ObjectList : public Yoga::Window
{
public:
    enum class Mode
    {
        Plater,
        Preview,
    };

    explicit ObjectList(Yoga::Item* parent = nullptr);

    void init(Biz::ProjectInteractor* project_interactor, Mode mode);

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

private:

    void setup_ui_state();
    void process_dragging_start();
    void update_selection_from_scene();
    bool render_list(Domain::Vec2f size);
    void render_header(Domain::Vec2f pos, Domain::Vec2f size);
    bool render_config_containers();
    void render_group_name(const std::string& name);
    void render_all_beds_node();
    bool render_out_of_beds();
    void render_drop_target_area();
    bool render_bed_node(const Domain::BedInstance* bed, size_t config_container_id);
    bool render_object_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed = nullptr, bool is_sla_config = false);
    bool render_connectors_node(const Slic3r::ModelObject* object, size_t bed_id);
    bool render_volumes(const Slic3r::ModelObject* object, size_t bed_id, bool is_sla_config);
    void render_volume_node(const Slic3r::ModelVolume* volume, const Domain::ElementRef& sel_element, bool is_selected, bool is_sla_config);
    bool render_instances_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed);
    bool render_instances(const Slic3r::ModelObject* object, const std::set<size_t>& instances_on_bed);
    void render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, bool is_selected);
    void render_infos_node(const Slic3r::ModelObject* object, bool is_sla_config);

    void render_edited(const char* init_name, const Domain::ElementRef& sel_element);
    void render_printable_icon(const Domain::ElementRef& sel_element, bool is_printable);
    void render_extruder_marker(size_t extruder_id, const std::vector<std::string>& colors);
    void render_slicing_state_marker(size_t bed_instance_id);
    void render_infos_selectable(const std::set<wchar_t>& infos, const Slic3r::ModelObject* object, bool force_render);

    bool tree_node(const char* str_id, ImGuiTreeNodeFlags flags, const std::string& label, bool add_overrides_marker = false);

    void clear_all_ms();
    void invalidate_bed_selection();

    bool handle_selection(const Domain::ElementRef& id);
    void handle_dragging(const Domain::ElementRef& id);
    void force_select_whole_object(const Slic3r::ModelObject* object);
    void bold_text(const std::string& text);

    void propagate_selection();
    void propagate_name_editing(const Domain::ElementRef& id, const std::string& new_name);
    void propagate_printable(const Domain::ElementRef& id, bool is_printable);
    void ask_extract_selected_instances();
    void extruder_clicked(const Domain::ElementRef& sel_element, bool is_bed);
    void show_layer_ranges(const Domain::ElementRef& id);
    void show_gizmo(const Domain::ElementRef& id, wchar_t gizmo_id);

    void render_scene_map(Domain::Vec2f size);
    void render_sliced_info(float height);

private:
    Mode                            m_mode              { Mode::Plater };
    Biz::ProjectInteractor*         m_project_interactor{ nullptr };
    Biz::Scene::SceneInteractor*    m_scene_interactor  { nullptr };
    const Slic3r::Model*            m_model             { nullptr };

    MultiSelections                 m_instances_ms;
    MultiSelections                 m_volumes_ms;
    size_t                          m_edited_node_id              { 0 };
    bool                            m_show_details                { false };
    bool                            m_scene_map                   { false };

    bool                            m_is_dragging                 { false };
    std::set<Domain::ElementRef>    m_selected_items;  // Track selected item IDs
    size_t                          m_selected_container_id       { 0 };
    size_t                          m_selected_bed_instance_id    { 0 };
    bool                            m_is_edit_name_input_hovered  { false };

    Domain::Vec2f                   m_inner_padding;
    ImGuiMultiSelectFlags           m_multi_selection_flags;
    ImGuiTreeNodeFlags              m_node_flags;
    ImGuiTableFlags                 m_table_flags;
};

} // namespace Slic3r::App
