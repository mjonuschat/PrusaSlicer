#include "Slic3r/App/Plater/ObjectList.hpp"
#include "Slic3r/Log.hpp"
//#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

#include <libslic3r/Model.hpp>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <boost/nowide/convert.hpp>
#include <set>

#include <vector>
#include <set>

namespace Slic3r {

std::set<Domain::ElementRef> selectedItems;  // Track selected item IDs
bool isDragging                     { false };

bool can_edit = true;

bool is_component_selected              = false;
bool is_height_range_modifier_selected  = false;
bool is_infos_selected                  = false;

namespace App::Plater {

bool handle_selection(const Domain::ElementRef& id)
{
    // handle selection only when we are NOT dragging and on MouseRelease or PressEnter
    if (!isDragging && ImGui::IsItemHovered(ImGuiHoveredFlags_None) && 
        (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        if (ImGui::GetIO().KeyCtrl) {
            if (selectedItems.count(id))
                selectedItems.erase(id);  // Toggle deselect
            else
                selectedItems.insert(id); // Multi-select
        }
        else {
            selectedItems.clear();
            selectedItems.insert(id);     // Single-select
        }
        return true;
    }
    return false;
}

void handle_dragging(const Domain::ElementRef& id)
{
    // Detect dragging on any selected node
    if (selectedItems.count(id) && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        isDragging = true;
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

struct VolumeNode {
    Domain::ElementRef sel_element;
    std::string icon;
    std::string label;
    const ModelConfigObject& config;
    bool is_editing = false;  // Track if the node is in edit mode
};

std::vector<VolumeNode> volume_nodes;
bool render_volumes(const Slic3r::ModelObject* object, const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection, bool can_mark_as_selected)
{
    if (volume_nodes.size() != object->volumes.size()) {
        for (const Slic3r::ModelVolume* volume : object->volumes) {
            Domain::ElementRef sel_element{ object->id().id, object->instances[0]->id().id, volume->id().id };
            volume_nodes.push_back({ sel_element, icon_str(volume), (volume->name.empty() ? "Volume " + std::to_string(volume->id().id) : volume->name), volume->config });
        }
    }

    static char buffer[128] = "";

    bool is_changed_selection{ false };

    for (VolumeNode& node : volume_nodes) {
        ImGui::PushID(&node);  // Ensure unique ID

        if (node.is_editing) {
            ImGui::Text(node.icon.c_str());
            ImGui::SameLine();
            // Editable text box
            strncpy(buffer, node.label.c_str(), sizeof(buffer));
            if (ImGui::InputText("##edit", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.label = buffer;  // Save edited name
                node.is_editing = false;  // Exit edit mode
            }
        }
        else {
            bool is_selected = can_mark_as_selected && is_volume_selected(node.sel_element, selection);
            // Display as a selectable label
            if (ImGui::Selectable((node.icon + node.label).c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
                // discard editing flag for all others
                for (VolumeNode& node_other : volume_nodes)
                    node_other.is_editing = false;

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    node.is_editing = true;  // Start edit mode on selection
                }

                is_changed_selection |= handle_selection(node.sel_element);
            }

            ImGui::NextColumn();
            ImGui::NextColumn();
            if (!node.config.empty()) {
                std::string icon = icon_str(ImGui::PrintIconMarker);
                ImGui::Text(icon.c_str());
            }
            ImGui::NextColumn();
        }

        ImGui::PopID();
    }

    return is_changed_selection;
}

bool render_volume_node(const Slic3r::ModelVolume* volume, const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection, bool can_mark_as_selected)
{
    const Slic3r::ModelObject* object = volume->get_object();
    size_t id = volume->id().id;
    Domain::ElementRef sel_element{ object->id().id, object->instances[0]->id().id, id};

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
    if (can_mark_as_selected && is_volume_selected(sel_element, selection)) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = icon_str(volume) + (volume->name.empty() ? "Volume " + std::to_string(id) : volume->name);
    const std::string name_id = "##vol_id" + std::to_string(volume->id().id);

    bool is_open = ImGui::TreeNodeEx(name_id.c_str(), flags, "%s", name.c_str());

    bool is_changed_selection = handle_selection(sel_element);
    handle_dragging(sel_element);

    ImGui::NextColumn();
    ImGui::NextColumn();
    if (!volume->config.empty()) {
        std::string icon = icon_str(ImGui::PrintIconMarker);
        ImGui::Text(icon.c_str());
    }
    ImGui::NextColumn();
    
    if (is_open)
        ImGui::TreePop();

    return is_changed_selection;
}

bool render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection, bool can_mark_as_selected)
{
    const Slic3r::ModelInstance* instance = object->instances[inst_id];
    size_t id = instance->id().id;
    Domain::ElementRef sel_element{ object->id().id, id, 0 };

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
    if (can_mark_as_selected && selection.is_selected(sel_element)) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = icon_str(ImGui::ObjectIcon) + "Instance " + std::to_string(inst_id + 1);
    const std::string name_id = "##inst_id" + std::to_string(id);

    bool is_open = ImGui::TreeNodeEx(name_id.c_str(), flags, "%s", name.c_str());

    bool is_changed_selection = handle_selection(sel_element);
    handle_dragging(sel_element);

    ImGui::NextColumn();
    std::string icon = icon_str(instance->printable ? ImGui::EyeOpen : ImGui::EyeClosed);
    ImGui::Text(icon.c_str());

    // if clicked => send event to the project about changes

    ImGui::NextColumn();
    ImGui::NextColumn();
    
    if (is_open)
        ImGui::TreePop();

    return is_changed_selection;
}

void force_select_whole_object(const Slic3r::ModelObject* object)
{
    size_t id = object->id().id;
    Domain::ElementRef sel_element{ id, 0, 0 };
    if (selectedItems.count(sel_element)) {
        // remove object element
        selectedItems.erase(sel_element);
        //  and push all instances instad
        for (const Slic3r::ModelInstance* instance : object->instances)
            selectedItems.insert({ id, instance->id().id, 0 });
    }
}

bool render_instances_node(const Slic3r::ModelObject* object, const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection, bool is_whole_object_selected)
{
    size_t id = object->id().id;
    Domain::ElementRef sel_element{ id, 0, 0 };

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
//    if (selectedItems.count(sel_element)) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name_id = "Instances##obj_id" + std::to_string(object->id().id);

    bool isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags, "Component");

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection)
        force_select_whole_object(object);
    handle_dragging(sel_element);

    ImGui::NextColumn();
    ImGui::NextColumn();
    ImGui::NextColumn();

    if (isOpen) {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing() - GImGui->Style.FramePadding.x);
        for (size_t inst_id = 0; inst_id < object->instances.size(); inst_id++)
            is_changed_selection |= render_instance_node(object, inst_id, model, selection, !is_whole_object_selected);
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing() - GImGui->Style.FramePadding.x);
        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool render_object_node(const Slic3r::ModelObject* object, const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection)
{
    size_t id = object->id().id;
    Domain::ElementRef sel_element{ id, 0, 0 };

    bool is_selected = is_whole_object_selected(object, selection);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = icon_str(ImGui::ObjectIcon) + (object->name.empty() ? "Object " + std::to_string(id) : object->name);
    const std::string name_id = "##obj_id" + std::to_string(id);

    bool isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags, "%s", name.c_str());

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection)
        force_select_whole_object(object);
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
        if (object->volumes.size() > 1) {
            if (can_edit)
                is_changed_selection |= render_volumes(object, model, selection, !is_selected);
            else {
                ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing() - GImGui->Style.FramePadding.x);
                for (const Slic3r::ModelVolume* volume : object->volumes)
                    is_changed_selection |= render_volume_node(volume, model, selection, !is_selected);
                ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing() - GImGui->Style.FramePadding.x);
            }
        }



        if (object->instances.size() > 1)
            is_changed_selection |= render_instances_node(object, model, selection, is_selected);

        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool render_tree(const Slic3r::Model& model, const Slic3r::Biz::Scene::Selection& selection)
{
    ImGui::Columns(3, "tree", false);

    ImGui::SetColumnWidth(0, 300);
    ImGui::SetColumnWidth(1, 35);
    ImGui::SetColumnWidth(2, 35);

    bool is_changed_selection = false;
    for (const Slic3r::ModelObject* object : model.objects)
        is_changed_selection |= render_object_node(object, model, selection);

    ImGui::Columns(1);

    return is_changed_selection;
}

void ObjectList::render(ImVec2 pos, ImVec2 size)
{
    assert(m_model && m_scene_interactor);

    ImGui::SetCursorScreenPos(pos + ImVec2(10.f, 10.f));
    ImGui::Text("Objects");
    ImGui::SameLine();
    ImGui::Checkbox("edit volume names##checkb", &can_edit);

    isDragging = false;
    // Define a region for the tree control
    if (render_tree(*m_model, m_scene_interactor->selection())) {
        // update selection on scene
        propagate_selection();
    }

    // Start drag operation when any selected node is being dragged
    if (isDragging && ImGui::BeginDragDropSource(/*ImGuiDragDropFlags_SourceNoHoldToOpenOthers | */ImGuiDragDropFlags_SourceExtern)) {
        int size = (int)selectedItems.size();
        ImGui::Text("Dragging %d item(s)", size);
        ImGui::SetDragDropPayload("MULTI_ITEM", &size, sizeof(int));
        ImGui::EndDragDropSource();
    }

    // Drop Target Area
    ImGui::Separator();
    ImGui::Text("Drop Here:");

//    ImGui::SameLine();

    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(25.f, 0.f));
    static std::string out;
    ImGui::Text(out.c_str());

    // Make the entire window a valid drop target
    ImVec2 dropAreaSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("DropZone", ImVec2(dropAreaSize.x, 200.f));  // Creates an invisible drop target

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MULTI_ITEM")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int*)payload->Data;
            out = "Dropped " + std::to_string(payload_n) + " item(s)!";
            selectedItems.clear();  // Clear selection after drop
        }
        else
            out.clear();
        ImGui::EndDragDropTarget();
    }
}

void ObjectList::propagate_selection()
{    
    Biz::Scene::Selection sels;
    sels.elements = std::vector<Domain::ElementRef>(selectedItems.begin(), selectedItems.end());
    sels.normalize();
    m_scene_interactor->set_selection(sels);
}

}// App::Plater namespace

}// Slic3r namespace
