#pragma once
#include "imgui/imgui.h"
#include "Slic3r/Domain/Types.hpp"

#include <map>
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
class BedInstance;
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

struct MultiSelectionStorage : public ImGuiSelectionBasicStorage // !!! move into separate files with imgui includes
{
    // override ApplyRequests to check if selection was changed
    void ApplyRequests(ImGuiMultiSelectIO* ms_io);

    bool    is_changed                  { false };
    
private:
    size_t  last_size                   { 0 };
    size_t  last_single_selected_id     { 0 };
    bool    is_started                  { true };
};

/**
 * @brief Help class to save several instances of MultiSelectionStorage for each set of volums or instances of the object
 * @param key is always Id of the object 
 * @param value is a MultiSelectionStorage of the selected volums or instances of this object.
 */
class MultiSelections : public std::map<size_t, MultiSelectionStorage>
{
public:
    // T may be ModelInstancePtrs OR ModelVolumePtrs
    template <typename T>
    MultiSelectionStorage& get_ms(size_t object_id)
    {
        if (this->find(object_id) == this->end()) {
            this->emplace(object_id, MultiSelectionStorage());
            this->at(object_id).AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self, int idx) {
                return (ImGuiID)((T*)self->UserData)->at(idx)->id().id;
            };
        }

        return this->at(object_id);
    }

    void clear_all();
    void clear_except(size_t id);
};

class ObjectList
{
public:
    ObjectList() {}
    
    void init(Biz::ProjectInteractor* project_interactor, Render::ImguiRender* imgui_render) {
        m_project_interactor = project_interactor;
        m_imgui_render = imgui_render;
    }

    void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:

    void update_selection_from_scene();
    bool render_tree(Domain::Vec2f size);
    void render_header(Domain::Vec2f pos, Domain::Vec2f size);
    bool render_config_containers();
    bool render_out_of_beds();
    bool render_bed_node(const Domain::BedInstance* bed, bool is_sla_config);
    bool render_object_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed = nullptr, bool is_sla_config = false);
    bool render_connectors_node(const Slic3r::ModelObject* object, size_t bed_id);
    bool render_volumes(const Slic3r::ModelObject* object, size_t bed_id);
    void render_volume_node(const Slic3r::ModelVolume* volume, size_t vol_id, bool is_selected, const Domain::ElementRef& sel_element);
    bool render_instances_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed);
    bool render_instances(const Slic3r::ModelObject* object, const std::set<size_t>& instances_on_bed);
    void render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, bool is_selected);
    void render_infos_node(const Slic3r::ModelObject* object, bool is_sla_config);

    void render_edited(const char* init_name, const Domain::ElementRef& sel_element);
    void render_printable_icon(const Domain::ElementRef& sel_element, bool is_printable);
    void render_overrides_icon(const Domain::ElementRef& sel_element, bool render);
    void render_extruder_marker(size_t extruder_id, const Domain::ElementRef& sel_element, bool is_bed = false);
    void render_infos_selectable(const std::set<wchar_t>& infos, const Slic3r::ModelObject* object, bool force_render);

    void clear_all_ms();

    void propagate_selection();
    void propagate_name_editing(const Domain::ElementRef& id, const std::string& new_name);
    void propagate_printable(const Domain::ElementRef& id, bool is_printable);
    void ask_extract_selected_instances();
    void show_overrides(const Domain::ElementRef& id);
    void extruder_clicked(const Domain::ElementRef& sel_element, bool is_bed);
    void show_layer_ranges(const Domain::ElementRef& id);
    void show_gizmo(const Domain::ElementRef& id, wchar_t gizmo_id);

private:
    Biz::ProjectInteractor*         m_project_interactor{ nullptr };
    Biz::Scene::SceneInteractor*    m_scene_interactor  { nullptr };
    const Slic3r::Model*            m_model             { nullptr };
    Render::ImguiRender*            m_imgui_render      { nullptr };

    MultiSelections                 m_instances_ms;
    MultiSelections                 m_volumes_ms;

    size_t                          m_edited_node_id    { 0 };
    bool                            m_show_details      { false };
};

} // namespace Slic3r::App
