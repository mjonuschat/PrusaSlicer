#include "Slic3r/App/Plater/ObjectList.hpp"
#include "Slic3r/Log.hpp"
//#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <libslic3r/Model.hpp>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <boost/nowide/convert.hpp>
#include <set>

#include <vector>
#include <set>

namespace Slic3r::App::Plater {

void MultiSelectionStorage::ApplyRequests(ImGuiMultiSelectIO* ms_io)
{
    ImGuiSelectionBasicStorage::ApplyRequests(ms_io);
    if (is_started) {
        last_size = Size;
        last_single_selected_id = Size == 1 ? _Storage.Data.begin()->key : 0;
        is_changed = false;
    }
    else
        is_changed = last_size != Size || (Size == 1 && last_single_selected_id != _Storage.Data.begin()->key);

    is_started = !is_started;
}

void MultiSelections::clear_all()
{
    for (auto& [obj_id, ms] : *this)
        ms.Clear();
}

void MultiSelections::clear_except(size_t id)
{
    for (auto& [obj_id, ms] : *this)
        if (obj_id != id)
            ms.Clear();
}

std::set<Domain::ElementRef> selected_items;  // Track selected item IDs
bool is_dragging                     { false };

// hendle selection from the tree nodes
bool handle_selection(const Domain::ElementRef& id)
{
    // handle selection only when we are NOT dragging and on MouseRelease or PressEnter
    if (!is_dragging && ImGui::IsItemHovered(ImGuiHoveredFlags_None) && 
        (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        if (ImGui::GetIO().KeyCtrl) {
            if (selected_items.count(id))
                selected_items.erase(id);  // Toggle deselect
            else
                selected_items.insert(id); // Multi-select
        }
        else {
            selected_items.clear();
            selected_items.insert(id);     // Single-select
        }
        return true;
    }
    return false;
}

void handle_dragging(const Domain::ElementRef& id)
{
    // Detect dragging on any selected node
    if (selected_items.count(id) && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        is_dragging = true;
}

std::string icon_str(const wchar_t icon)
{
    return boost::nowide::narrow(std::wstring(&icon, 1));
}

std::string icon_str(const Slic3r::ModelVolume* volume)
{
    if (volume->is_text()) {
        switch (volume->type()) {
        case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::TextSolidPartVolum);
        case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::TextNegativeVolume);
        case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::TextModifierVolum );
        }
        return "";    
    }
    if (volume->is_svg()) {
        switch (volume->type()) {
        case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::SvgSolidPartVolum);
        case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::SvgNegativeVolume);
        case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::SvgModifierVolum );
        }
        return "";    
    }

    switch (volume->type()) {
    case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::SolidPartVolum );
    case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::NegativeVolume );
    case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::ModifierVolum  );
    case Slic3r::ModelVolumeType::SUPPORT_BLOCKER   : return icon_str(ImGui::SupportBlocker );
    case Slic3r::ModelVolumeType::SUPPORT_ENFORCER  : return icon_str(ImGui::SupportModifier);
    default: 
        return "";
    }
}

bool is_whole_object_selected(const Slic3r::ModelObject* object, const Slic3r::Biz::Scene::Selection& selection)
{
    if (selection.mode == Biz::Scene::SelectionMode::Instance) {
        size_t cnt = 0;
        size_t object_id = object->id().id;
        for (const Domain::ElementRef& el : selection.elements)
            if (el.object_id == object_id)
                cnt++;
        return cnt == object->instances.size();
    }
    return false;
}

bool is_volume_selected(const Domain::ElementRef& sel_element, const Slic3r::Biz::Scene::Selection& selection)
{
    if (selection.mode == Biz::Scene::SelectionMode::Volume) {
        for (const Domain::ElementRef& el : selection.elements)
            if (el.object_id == sel_element.object_id && el.volume_id == sel_element.volume_id)
                return true;
    }

    return false;
}


ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ScopeRect | 
                                 ImGuiMultiSelectFlags_ClearOnEscape |
                                 ImGuiMultiSelectFlags_BoxSelect1d | 
                                 ImGuiMultiSelectFlags_SelectOnClick;

size_t edited_node_id = 0;
//char buffer[128] = "";

// render edited item as an input text and propagate new name to scene_interactor
void ObjectList::render_edited(const char* init_name, const Domain::ElementRef& sel_element)
{
    static char buffer[128] = "";

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
    ImGui::SameLine();
    ImGui::PopStyleVar();
    // Editable text box
    strncpy(buffer, init_name, sizeof(buffer));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    if (ImGui::InputText("##edit", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        propagate_name_editing(sel_element, buffer);  // Save edited name
        edited_node_id = 0;  // Exit edit mode
    }
    ImGui::PopStyleVar(2);
}

void ObjectList::render_volume(const Slic3r::ModelVolume* volume, size_t vol_id, bool is_selected, const Domain::ElementRef& sel_element)
{    
    std::string volume_name = (volume->name.empty() ? "Volume " + std::to_string(volume->id().id) : volume->name);
    size_t volume_id = volume->id().id;

    if (edited_node_id == volume_id && !is_selected)
        edited_node_id = 0;  // Exit edit mode

    ImGui::SetNextItemSelectionUserData(vol_id);
    if (edited_node_id == volume_id/* && ms.Size == 1*/) {
        if (ImGui::Selectable(icon_str(volume).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
            edited_node_id = 0;// Discard edit mode on selection
        }
        render_edited(volume_name.c_str(), sel_element);
    }
    else {
        // Display as a selectable label
        if (ImGui::Selectable((icon_str(volume) + volume_name).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
            edited_node_id = volume_id;  // Start edit mode on selection
    }

    ImGui::NextColumn();
    ImGui::NextColumn();
    if (!volume->config.empty()) {
        std::string icon = icon_str(ImGui::PrintIconMarker);
        ImGui::Text(icon.c_str());
    }
    ImGui::NextColumn();
}

bool ObjectList::render_volumes(const Slic3r::ModelObject* object)
{
    size_t object_id = object->id().id;
    size_t instance_id = object->instances[0]->id().id;

    const Slic3r::ModelVolumePtrs& volumes = object->volumes;

    MultiSelectionStorage& ms = m_volumes_ms.at(object_id);
    ImGui::PushID(&ms);  // Ensure unique ID
    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, ms.Size, volumes.size());
    ms.ApplyRequests(ms_io);

    for (size_t vol_id = 0; vol_id < volumes.size(); vol_id++) {
        const Slic3r::ModelVolume* volume = object->volumes[vol_id];
        size_t volume_id = volume->id().id;
        render_volume(volume, vol_id, ms.Contains((ImGuiID)volume_id), { object_id, instance_id, volume_id });
    }

    // Apply multi-select requests
    ms_io = ImGui::EndMultiSelect();
    ms.ApplyRequests(ms_io);
    ImGui::PopID();

    // update selection
    if (ms.is_changed) {
        m_instances_ms.clear_all();
        m_volumes_ms.clear_except(object_id);

        std::set<Domain::ElementRef> selectedItems_tmp;
        for (const Slic3r::ModelVolume* volume : volumes) {
            size_t volume_id = volume->id().id;
            if (ms.Contains((ImGuiID)volume_id))
                selectedItems_tmp.insert({ object_id, instance_id, volume_id });
        }

        if (selected_items != selectedItems_tmp) {
            selected_items = selectedItems_tmp;
            return true;
        }
    }

    return false;
}

void ObjectList::render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, bool is_selected)
{
    const Slic3r::ModelInstance* instance = object->instances[inst_id];
    size_t id = instance->id().id;
    Domain::ElementRef sel_element{ object->id().id, id, 0 };

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Leaf;
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = icon_str(ImGui::ObjectIcon) + "Instance " + std::to_string(/*inst_id + 1*/id);
    
    ImGui::SetNextItemSelectionUserData(inst_id);
    ImGui::Selectable(name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns);

    handle_dragging(sel_element);

    ImGui::NextColumn();
    std::string icon = icon_str(instance->printable ? ImGui::EyeOpen : ImGui::EyeClosed);
    ImGui::Text(icon.c_str());

    // if clicked => send event to the project about changes

    ImGui::NextColumn();
    ImGui::NextColumn();
}

void force_select_whole_object(const Slic3r::ModelObject* object)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };
    if (selected_items.count(sel_element)) {
        // remove object element
        selected_items.erase(sel_element);
        //  and push all instances instad
        for (const Slic3r::ModelInstance* instance : object->instances)
            selected_items.insert({ object_id, instance->id().id });
    }
}

bool ObjectList::render_instances_node(const Slic3r::ModelObject* object)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };

    MultiSelectionStorage& ms = m_instances_ms.at(object_id);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    const std::string name_id = "Instances##obj_id" + std::to_string(object->id().id);

    bool isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags, "Component");

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection) {
        force_select_whole_object(object);
        m_volumes_ms.clear_all();
        if (ImGui::GetIO().KeyCtrl)
            ms.Clear();
        else
            m_instances_ms.clear_all();
    }

    handle_dragging(sel_element);

    ImGui::NextColumn();
    ImGui::NextColumn();
    ImGui::NextColumn();

    if (isOpen) {
        ImGui::PushID(&ms);
        
        ImGuiMultiSelectIO* ms_inst_io = ImGui::BeginMultiSelect(ms_flags, ms.Size, object->instances.size());
        ms.ApplyRequests(ms_inst_io);

        for (size_t inst_id = 0; inst_id < object->instances.size(); inst_id++)
            render_instance_node(object, inst_id, ms.Contains((ImGuiID)object->instances[inst_id]->id().id));

        ms_inst_io = ImGui::EndMultiSelect();
        ms.ApplyRequests(ms_inst_io);
        ImGui::PopID();
        
        if (ms.is_changed) {
            m_volumes_ms.clear_all();

            if (!ImGui::GetIO().KeyCtrl) {
                m_instances_ms.clear_except(object_id);
                selected_items.clear();
            }

            for (const Slic3r::ModelInstance* instance : object->instances) {
                size_t instance_id = instance->id().id;
                Domain::ElementRef sel_element{ object_id, instance->id().id };

                if (ms.Contains((ImGuiID)instance_id) && !selected_items.count(sel_element)) {
                    is_changed_selection = true;
                    selected_items.insert(sel_element);
                }
                else if (!ms.Contains((ImGuiID)instance_id) && selected_items.count(sel_element)) {
                    is_changed_selection = true;
                    selected_items.erase(sel_element);
                }
            }            
        }

        ImGui::TreePop();
    }

    return is_changed_selection;
}

void ObjectList::clear_all_ms()
{
    m_instances_ms.clear_all();
    m_volumes_ms.clear_all();
}

bool ObjectList::render_object_node(const Slic3r::ModelObject* object, const Slic3r::Biz::Scene::Selection& selection)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };

    bool is_selected = is_whole_object_selected(object, selection);
    if (edited_node_id == object_id && !is_selected)
        edited_node_id = 0;  // Exit edit mode

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = (object->name.empty() ? "Object " + std::to_string(object_id) : object->name);
    const std::string name_id = "##obj_id" + std::to_string(object_id);

    bool isOpen = false; 
    if (edited_node_id == object_id && is_selected) {
        isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags | ImGuiTreeNodeFlags_AllowOverlap, "%s", icon_str(ImGui::ObjectIcon).c_str());
        render_edited(name.c_str(), { object_id });
    }
    else
        isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags, "%s", (icon_str(ImGui::ObjectIcon) + name).c_str());

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection) {

        force_select_whole_object(object);
        m_volumes_ms.clear_all();
        if (ImGui::GetIO().KeyCtrl)
            m_instances_ms.at(object_id).Clear();
        else
            m_instances_ms.clear_all();
        edited_node_id = object_id;
    }

    handle_dragging(sel_element);

    ImGui::NextColumn();
    std::string icon = icon_str(object->printable ? ImGui::EyeOpen : ImGui::EyeClosed);
    ImGui::Text(icon.c_str());
    ImGui::NextColumn();
    if (!object->config.empty()) {
        std::string icon = icon_str(ImGui::PrintIconMarker);
        ImGui::Text(icon.c_str());
    }
    ImGui::NextColumn();

    if (isOpen) {
        if (object->volumes.size() > 1)
            is_changed_selection |= render_volumes(object);

        if (object->instances.size() > 1)
            is_changed_selection |= render_instances_node(object);

        ImGui::TreePop();
    }

    return is_changed_selection;
}

void ObjectList::update_selection_from_scene(const Slic3r::Biz::Scene::Selection& scene_selection)
{
    for (const Slic3r::ModelObject* object : m_model->objects) {
        size_t object_id = object->id().id;

        MultiSelectionStorage& inst_ms = m_instances_ms.get_ms<Slic3r::ModelInstancePtrs>(object_id);
        inst_ms.UserData = (void*)&object->instances;

        MultiSelectionStorage& vol_ms = m_volumes_ms.get_ms<Slic3r::ModelVolumePtrs>(object_id);
        vol_ms.UserData = (void*)&object->volumes;
    }

    if (!scene_selection.empty()) {
        std::set<Domain::ElementRef> selectedItems_tmp = std::set<Domain::ElementRef>(scene_selection.elements.begin(), scene_selection.elements.end());
        if (selectedItems_tmp != selected_items) {
            clear_all_ms();
            if (scene_selection.mode == Biz::Scene::SelectionMode::Volume) {
                for (const auto& el : scene_selection.elements)
                    m_volumes_ms.at(el.object_id).SetItemSelected(el.volume_id, true);
            }
            else if (scene_selection.mode == Biz::Scene::SelectionMode::Instance) {
                for (const auto& el : scene_selection.elements) {
                    m_instances_ms.at(el.object_id).SetItemSelected(el.instance_id, true);
                }
            }
            selected_items = selectedItems_tmp;
        }
    }
}

bool ObjectList::render_tree(ImVec2 size, const Slic3r::Biz::Scene::Selection& scene_selection)
{
    bool is_changed_selection = false;
    if (ImGui::BeginChild("##Tree", ImVec2(size.x, size.y - ImGui::GetFontSize() * 2), ImGuiChildFlags_ResizeY))
    {
        ImGui::Columns(3, "tree", false);

        ImGui::SetColumnWidth(0, 300);
        ImGui::SetColumnWidth(1, 35);
        ImGui::SetColumnWidth(2, 35);

        for (const Slic3r::ModelObject* object : m_model->objects)
            is_changed_selection |= render_object_node(object, scene_selection);
    }
    ImGui::EndChild();

    ImGui::Columns(1);

    return is_changed_selection;
}

void ObjectList::render(ImVec2 pos, ImVec2 size)
{
    assert(m_model && m_scene_interactor);
    update_selection_from_scene(m_scene_interactor->selection());

    ImGui::SetCursorScreenPos(pos + ImVec2(10.f, 10.f));
    ImGui::Text("Objects");

    is_dragging = false;
    // Define a region for the tree control
    if (render_tree(size, m_scene_interactor->selection())) {
        // update selection on scene
        propagate_selection();
    }

    // Start drag operation when any selected node is being dragged
    if (is_dragging && ImGui::BeginDragDropSource(/*ImGuiDragDropFlags_SourceNoHoldToOpenOthers | */ImGuiDragDropFlags_SourceExtern)) {
        int size = (int)selected_items.size();
        ImGui::Text("Dragging %d item(s)", size);
        ImGui::SetDragDropPayload("MULTI_ITEM", &size, sizeof(int));
        ImGui::EndDragDropSource();
    }

    // Drop Target Area
    ImGui::Separator();
    ImGui::Text("Drop Here:");

    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(25.f, 0.f));
    static std::string out;
    ImGui::Text(out.c_str());

    // Make the entire window a valid drop target
    ImVec2 dropAreaSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("DropZone", ImVec2(dropAreaSize.x, 100.f));  // Creates an invisible drop target

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MULTI_ITEM")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int*)payload->Data;
            out = "Dropped " + std::to_string(payload_n) + " item(s)!";
            selected_items.clear();  // Clear selection after drop
        }
        else
            out.clear();
        ImGui::EndDragDropTarget();
    }
}

void ObjectList::propagate_selection()
{    
    Biz::Scene::Selection sels;
    sels.elements = std::vector<Domain::ElementRef>(selected_items.begin(), selected_items.end());
    sels.normalize();
    m_scene_interactor->set_selection(sels);
}

void ObjectList::propagate_name_editing(const Domain::ElementRef& id, const std::string& new_name)
{
    // ask project interactor to rename element with object_id
    m_scene_interactor->edit_name(id, new_name);
}

}// Slic3r::App::Plater namespace
