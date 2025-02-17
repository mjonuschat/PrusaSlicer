#pragma once
#include "imgui/imgui.h"

#include <map>
#include <string>

namespace Slic3r {
class Model;
class ModelObject;
class ModelInstance;
class ModelVolume;
}

namespace Slic3r::Domain {
struct ElementRef;
}

namespace Slic3r::Biz::Scene {
class SceneInteractor;
struct Selection;
}

namespace Slic3r::App::Plater {

struct MultiSelectionStorage : public ImGuiSelectionBasicStorage
{
    void ApplyRequests(ImGuiMultiSelectIO* ms_io);

    bool    is_changed                  { false };
    
private:
    size_t  last_size                   { 0 };
    size_t  last_single_selected_id     { 0 };
    bool    is_started                  { true };
};

//                                      object_id  Selections storage
class MultiSelections : public std::map<size_t, MultiSelectionStorage>
{
public:

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
    
    void init(Biz::Scene::SceneInteractor& scene_interactor, const Slic3r::Model& model) {
        m_scene_interactor = &scene_interactor;
        m_model = &model;
    }

    void render(ImVec2 pos, ImVec2 size);

protected:

private:

    void update_selection_from_scene(const Slic3r::Biz::Scene::Selection& selection);
    bool render_tree(ImVec2 size, const Slic3r::Biz::Scene::Selection& selection);
    bool render_object_node(const Slic3r::ModelObject* object, const Slic3r::Biz::Scene::Selection& selection);
    bool render_instances_node(const Slic3r::ModelObject* object);
    void render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, bool is_selected);
    void render_volume(const Slic3r::ModelVolume* volume, size_t vol_id, bool is_selected, const Domain::ElementRef& sel_element);
    bool render_volumes(const Slic3r::ModelObject* object);

    void render_edited(const char* init_name, const Domain::ElementRef& sel_element);

    void clear_all_ms();

    void propagate_selection();
    void propagate_name_editing(const Domain::ElementRef& id, const std::string& new_name);

private:
    Biz::Scene::SceneInteractor*    m_scene_interactor  { nullptr };
    const Slic3r::Model*            m_model             { nullptr };

    MultiSelections m_instances_ms;
    MultiSelections m_volumes_ms;
};

} // namespace Slic3r::App::Plater