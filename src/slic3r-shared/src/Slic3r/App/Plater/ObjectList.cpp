#include "Slic3r/App/Plater/ObjectList.hpp"
#include "Slic3r/Log.hpp"
//#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
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

//tmp include
#include "libslic3r/format.hpp"

namespace Slic3r::App::Plater {

void MultiSelectionStorage::ApplyRequests(ImGuiMultiSelectIO* ms_io)
{
    // process ApplyRequests() from paarent class
    ImGuiSelectionBasicStorage::ApplyRequests(ms_io);

    // ApplyRequests alwys is called twise (for Begin and End MultipleSelection)
    if (is_started) {
        // So, always apply last_size && last_single_selected_id values on the Begin 
        last_size = Size;
        last_single_selected_id = Size == 1 ? _Storage.Data.begin()->key : 0;
        is_changed = false;
    }
    else {
        // And chech is something was changed at the End
        is_changed = last_size != Size || (Size == 1 && last_single_selected_id != _Storage.Data.begin()->key);
    }

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

enum ColumIndex
{
    ciTree = 0,
    ciPrintable,
    ciSettingsOvverides,
    ciExtruder,

    ciCount
};

ImGuiMultiSelectFlags   ms_flags      = ImGuiMultiSelectFlags_ScopeRect | 
                                        ImGuiMultiSelectFlags_ClearOnEscape |
                                        ImGuiMultiSelectFlags_BoxSelect1d | 
                                        ImGuiMultiSelectFlags_SelectOnClick;

ImGuiTreeNodeFlags      node_flags    = ImGuiTreeNodeFlags_OpenOnArrow | 
                                        ImGuiTreeNodeFlags_SpanAllColumns;

ImGuiTableFlags         table_flags   = ImGuiTableFlags_ScrollY | 
                                        ImGuiTableFlags_Resizable | 
                                        ImGuiTableFlags_NoBordersInBody | 
                                        ImGuiTableFlags_NoPadInnerX;

static ImVec4 def_color = ImVec4(0.923f, 0.504f, 0.264f, 1.0f);
static std::string test_out;
static std::string test_out2;

std::set<Domain::ElementRef> selected_items;  // Track selected item IDs
bool is_dragging                     { false };

// hendle selection from the tree nodes
static bool handle_selection(const Domain::ElementRef& id)
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

static void handle_dragging(const Domain::ElementRef& id)
{
    // Detect dragging on any selected node
    if (selected_items.count(id) && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        is_dragging = true;
}

static std::string icon_str(const wchar_t icon)
{
    return boost::nowide::narrow(std::wstring(&icon, 1));
}

static std::string icon_str(const Slic3r::ModelVolume* volume)
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

static std::string get_bed_name(const Slic3r::DynamicPrintConfig& print_config)
{
    return icon_str(Slic3r::Preset::printer_technology(print_config) == ptSLA ? 
                    ImGui::PrinterSlaIconMarker : ImGui::PrinterIconMarker) 
           + print_config.opt_string("printer_model");
}

static bool bed_has_object(const Domain::ModelInstanceList& bed_model_instances, const Slic3r::ModelObject* object)
{
    for (const Slic3r::ModelInstance* instance : bed_model_instances)
        if (instance->get_object() == object)
            return true;

    return false;
}

static std::set<size_t> get_object_instance_ids_on_bed(const Domain::ModelInstanceList& bed_model_instances, const Slic3r::ModelObject* object)
{
    std::set<size_t> object_instances_on_bed;

    for (Slic3r::ModelInstance* instance : bed_model_instances)
        if (instance->get_object() == object)
            object_instances_on_bed.emplace(instance->id().id);

    return object_instances_on_bed;
}

static bool is_whole_object_selected(const Slic3r::ModelObject* object, const Slic3r::Biz::Scene::Selection& selection)
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

static bool is_volume_selected(const Domain::ElementRef& sel_element, const Slic3r::Biz::Scene::Selection& selection)
{
    if (selection.mode == Biz::Scene::SelectionMode::Volume) {
        for (const Domain::ElementRef& el : selection.elements)
            if (el.object_id == sel_element.object_id && el.volume_id == sel_element.volume_id)
                return true;
    }

    return false;
}

static size_t visible_volumes_count(const ModelObject* object)
{
    if (object->is_cut()) {
        size_t count{ 0 };
        for (const auto* volume : object->volumes)
            if (!volume->is_cut_connector())
                count++;
        
        return count;
    }

    return object->volumes.size();
}

static void force_select_whole_object(const Slic3r::ModelObject* object)
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

std::set<wchar_t> get_infos(const Slic3r::ModelObject* object, bool is_sla_config)
{
    std::set<wchar_t> infos;
    if (!is_sla_config) {
        for (const ModelVolume* mv : object->volumes) {
            if (!mv->supported_facets.empty())
                infos.insert(ImGui::CustomSupports);
            if (!mv->seam_facets.empty())
                infos.insert(ImGui::CustomSeam);
            if (!mv->fuzzy_skin_facets.empty())
                infos.insert(ImGui::FuzzySkin);
            if (!mv->mm_segmentation_facets.empty())
                infos.insert(ImGui::MmSegmentation);
        }
        //if (wxGetApp().plater()->canvas3D()->is_object_sinking(obj_idx))
        //    infos.insert(ImGui::Sinking);
    }

    return infos;
}

static void next_line()
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
}

static bool hovered_current_row()
{
    // get cursor position for the first cell in the row
    ImGui::TableSetColumnIndex(ciTree);
    ImVec2 row_begin = ImGui::GetCursorScreenPos();
    // !Strange effect: It returns differs values for rows with Selectable() and TreeNode() controls
    // it looks like for Selectable() it's a LeftBottom of the cell instead of LeftTop.
    // So let's update row_begin.y value from the last cell of the row

    // get cursor position for the last cell in the row
    ImGui::TableSetColumnIndex(ciCount-1);
    ImVec2 row_end = ImGui::GetCursorScreenPos();
    row_begin.y = row_end.y; // update row_begin.y
    // get RightBottom for the row
    row_end += ImGui::CalcTextSize(icon_str(ImGui::PrintIconMarker).c_str());

    return ImGui::IsMouseHoveringRect(row_begin, row_end, false);
}

static  std::string info_line;
static bool open_modal = false;
static bool open_extra_window = false;
static void show_test_window(const std::string& line, bool show_as_modal = true)
{
    info_line = line;
    if (show_as_modal)
        open_modal = true;
    else
        open_extra_window = true;
}

static void render_modal()
{
    if (!open_modal)
        return;
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(430, 150), ImGuiCond_Always);

    std::string win_name = "ObjectList action##my modal";
    if (!ImGui::IsPopupOpen(win_name.c_str()))
        ImGui::OpenPopup(win_name.c_str());

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.5f, 20.5f));
    if (ImGui::BeginPopupModal(win_name.c_str(), &open_modal)) {
        ImGui::Text(info_line.c_str());
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

static void render_extra_window()
{
    if (!open_extra_window)
        return;
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(430, 100), ImGuiCond_Always);

    std::string win_name = "ObjectList action##my window";

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.5f, 20.5f));
    ImGui::Begin(win_name.c_str(), &open_extra_window);
    ImGui::Text(info_line.c_str());
    ImGui::End();
    ImGui::PopStyleVar();
}

static bool icon_btn(ColumIndex ci, const std::string& icon)
{
    ImGui::TableSetColumnIndex(ci);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    const float size = ImGui::GetFontSize();
    ImRect hovered_rc(pos, pos + ImVec2(size, size));

    ImGui::Text(icon.c_str());

    bool pressed = ImGui::IsMouseHoveringRect(hovered_rc.Min, hovered_rc.Max) && ImGui::IsMouseClicked(0);
    return pressed;
}

void ObjectList::render(ImVec2 pos, ImVec2 size)
{
    m_scene_interactor = &m_project_interactor->scene_interactor();
    m_model = &m_project_interactor->selected_project().model();

    assert(m_model && m_scene_interactor);
    update_selection_from_scene();

    ImGui::SetCursorScreenPos(pos + ImVec2(10.f, 10.f));
    ImGui::Text("Objects");

    is_dragging = false;
    // Define a region for the tree control
    if (render_tree(size)) {
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
    ImGui::Text(test_out.c_str());
    ImGui::Text(test_out2.c_str());

    // Make the entire window a valid drop target
    ImVec2 dropAreaSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("DropZone", ImVec2(dropAreaSize.x, 100.f));  // Creates an invisible drop target

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MULTI_ITEM")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int*)payload->Data;
            test_out = "Dropped " + std::to_string(payload_n) + " item(s)!";
            selected_items.clear();  // Clear selection after drop
        }
        else
            test_out.clear();
        ImGui::EndDragDropTarget();
    }

    render_modal();
    render_extra_window();
}

void ObjectList::update_selection_from_scene()
{
    const Slic3r::Biz::Scene::Selection& scene_selection = m_scene_interactor->selection();
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

bool ObjectList::render_tree(ImVec2 size)
{
    bool is_changed_selection = false;
    if (ImGui::BeginTable("MyTable", 4, table_flags)) {
        ImGui::TableSetupColumn("##tree", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##visibility", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableSetupColumn("##settings_overrides", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableSetupColumn("##extruder", ImGuiTableColumnFlags_WidthStretch/* | ImGuiTableColumnFlags_DefaultHide*/, 0.1f);

        is_changed_selection = render_config_containers();
        is_changed_selection |= render_out_of_beds();

        ImGui::EndTable();
    }

    return is_changed_selection;
}

bool ObjectList::render_config_containers()
{
    bool is_changed_selection = false;
    for (auto& cc : m_scene_interactor->selected_project_config_containers()) {
        next_line();
        if (ImGui::TreeNodeEx(("##cc_id" + std::to_string(cc->id().id)).c_str(), node_flags | ImGuiTreeNodeFlags_DefaultOpen,
                                         (get_bed_name(cc->print_config())).c_str())) {
            for (auto& bed_inst : cc->bed_instances())
                is_changed_selection |= render_bed_node(bed_inst.get(), cc->print_technology() == ptSLA);
            ImGui::TreePop();
        }
    }
    return is_changed_selection;
}

bool ObjectList::render_out_of_beds()
{
    if (m_scene_interactor->selected_project_unplaced_model_instances().empty())
        return false;

    next_line();
    bool is_changed_selection = false;

    if(ImGui::TreeNodeEx("##cc_out", node_flags | ImGuiTreeNodeFlags_DefaultOpen, "Out of bed")) {
        for (const Slic3r::ModelObject* object : m_model->objects) {
            if (bed_has_object(m_scene_interactor->selected_project_unplaced_model_instances(), object))
                is_changed_selection |= render_object_node(object);
        }

        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_bed_node(const Domain::BedInstance* bed, bool is_sla_config)
{
    size_t bed_id = bed->id().id;

    const std::string name = "Bed " + std::to_string(bed_id);
    const std::string name_id = "##bed_id" + std::to_string(bed_id);

    next_line();
    if (bed->active()) ImGui::PushStyleColor(ImGuiCol_Text, def_color);
    bool is_open = ImGui::TreeNodeEx(name_id.c_str(), node_flags | ImGuiTreeNodeFlags_DefaultOpen, (icon_str(ImGui::MinimalizeButton) + name).c_str());
    if (bed->active()) ImGui::PopStyleColor();

    render_extruder_marker(0, { bed_id }, true);
    
    bool is_changed_selection = false;
    if (is_open) {
        for (const Slic3r::ModelObject* object : m_model->objects) {
            if (bed_has_object(bed->model_instances(), object))
                is_changed_selection |= render_object_node(object, bed, is_sla_config);
        }
        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_object_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed /*= nullptr*/, bool is_sla_config /*= false*/)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };

    bool is_selected = is_whole_object_selected(object, m_scene_interactor->selection());
    if (m_edited_node_id == object_id && !is_selected)
        m_edited_node_id = 0;  // Exit edit mode

    ImGuiTreeNodeFlags flags = node_flags;
    if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

    const std::string name = (object->name.empty() ? "Object " + std::to_string(object_id) : object->name);
    const std::string name_id = "##obj_id" + std::to_string(object_id);

    next_line();

    bool isOpen = false; 
    if (m_edited_node_id == object_id && is_selected) {
        isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags | ImGuiTreeNodeFlags_AllowOverlap, icon_str(ImGui::ObjectIcon).c_str());
        render_edited(name.c_str(), { object_id });
    }
    else
        isOpen = ImGui::TreeNodeEx(name_id.c_str(), flags, (icon_str(ImGui::ObjectIcon) + name).c_str());

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection) {
        force_select_whole_object(object);
        m_volumes_ms.clear_all();
        if (ImGui::GetIO().KeyCtrl)
            m_instances_ms.at(object_id).Clear();
        else
            m_instances_ms.clear_all();
        m_edited_node_id = object_id;
    }

    handle_dragging(sel_element);

    render_printable_icon(sel_element, object->printable);
    render_overrides_icon(sel_element, !object->config.empty());
    render_extruder_marker(1, sel_element);

    if (isOpen) {
        is_changed_selection |= render_connectors_node(object, bed ? bed->id().id : 0);
        is_changed_selection |= render_volumes(object, bed ? bed->id().id : 0);

        if (object->instances.size() > 1) {
            std::set<size_t> instances_on_bed = get_object_instance_ids_on_bed(bed ? bed->model_instances() : m_scene_interactor->selected_project_unplaced_model_instances(), object);
            if (!instances_on_bed.empty())
                is_changed_selection |= render_instances_node(object, instances_on_bed);
        }

        is_changed_selection |= render_layer_ranges_node(object);
        render_infos_node(object, is_sla_config);

        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_connectors_node(const Slic3r::ModelObject* object, size_t bed_id)
{
    if (!object->is_cut() || object->volumes.size() == 1)
        return false;

    size_t object_id = object->id().id;
    size_t instance_id = object->instances[0]->id().id;
    const std::string name_id = "##connectors_id" + std::to_string(bed_id) + std::to_string(object_id);

    next_line();
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    if (ImGui::TreeNodeEx(name_id.c_str(), node_flags | ImGuiTreeNodeFlags_Leaf, (icon_str(ImGui::CutConnectors) + "Connectors").c_str()))
        ImGui::TreePop();
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    if (handle_selection({ object_id, instance_id })) {
        clear_all_ms();

        std::set<Domain::ElementRef> selectedItems_tmp;
        for (const Slic3r::ModelVolume* volume : object->volumes) {
            if (volume->is_cut_connector())
                selectedItems_tmp.insert({ object_id, instance_id, volume->id().id });
        }
        if (selected_items != selectedItems_tmp) {
            selected_items = selectedItems_tmp;
            return true;
        }
    }

    return false;
}

bool ObjectList::render_volumes(const Slic3r::ModelObject* object, size_t bed_id)
{
    if (visible_volumes_count(object) < 2)
        return false;

    size_t object_id = object->id().id;
    size_t instance_id = object->instances[0]->id().id;

    const std::string name_id = "##volumes_id" + std::to_string(bed_id) + std::to_string(object_id);

    next_line();
    bool is_open = ImGui::TreeNodeEx(name_id.c_str(), node_flags, "Volumes");

    const Slic3r::ModelVolumePtrs& volumes = object->volumes;
    MultiSelectionStorage& ms = m_volumes_ms.at(object_id);

    if (is_open) {
        ImGui::PushID(&ms);  // Ensure unique ID
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, ms.Size, visible_volumes_count(object));
        ms.ApplyRequests(ms_io);

        for (size_t vol_id = 0; vol_id < volumes.size(); vol_id++) {
            const Slic3r::ModelVolume* volume = object->volumes[vol_id];
            if (volume->is_cut_connector())
                continue;
            size_t volume_id = volume->id().id;
            render_volume_node(volume, vol_id, ms.Contains((ImGuiID)volume_id), { object_id, instance_id, volume_id });
        }

        ms_io = ImGui::EndMultiSelect();
        ms.ApplyRequests(ms_io);
        ImGui::PopID();
        
        ImGui::TreePop();
    }
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

// render edited item as an input text and propagate new name to scene_interactor
void ObjectList::render_volume_node(const Slic3r::ModelVolume* volume, size_t vol_id, bool is_selected, const Domain::ElementRef& sel_element)
{    
    std::string volume_name = (volume->name.empty() ? "Volume " + std::to_string(volume->id().id) : volume->name);
    size_t volume_id = volume->id().id;

    if (m_edited_node_id == volume_id && !is_selected)
        m_edited_node_id = 0;  // Exit edit mode

    next_line();
    ImGui::SetNextItemSelectionUserData(vol_id);
    if (m_edited_node_id == volume_id) {
        if (ImGui::Selectable(icon_str(volume).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
            m_edited_node_id = 0;// Discard edit mode on selection
        }
        render_edited(volume_name.c_str(), sel_element);
    }
    else {
        // Display as a selectable label
        if (ImGui::Selectable((icon_str(volume) + volume_name).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
            m_edited_node_id = volume_id;  // Start edit mode on selection
    }

    render_overrides_icon(sel_element, !volume->config.empty());
    render_extruder_marker(2, sel_element);
}

bool ObjectList::render_instances_node(const Slic3r::ModelObject* object, const std::set<size_t>& instances_on_bed)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };

    MultiSelectionStorage& ms = m_instances_ms.at(object_id);

    const std::string name_id = "Instances##obj_id" + std::to_string(object_id);

    next_line();
    bool isOpen = ImGui::TreeNodeEx(name_id.c_str(), node_flags | ImGuiTreeNodeFlags_DefaultOpen, "Instances");

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

    if (isOpen) {
        is_changed_selection |= render_instances(object, instances_on_bed);
        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_instances(const Slic3r::ModelObject* object, const std::set<size_t>& instances_on_bed)
{
    const Slic3r::ModelInstancePtrs& instances = object->instances;
    size_t object_id = object->id().id;

    MultiSelectionStorage& ms = m_instances_ms.at(object_id);
    ImGui::PushID(&ms);        
    ImGuiMultiSelectIO* ms_inst_io = ImGui::BeginMultiSelect(ms_flags, ms.Size, instances_on_bed.size());
    ms.ApplyRequests(ms_inst_io);

    for (size_t inst_id = 0; inst_id < instances.size(); inst_id++) {
        size_t instance_id = instances[inst_id]->id().id;
        if (instances_on_bed.count(instance_id))
            render_instance_node(object, inst_id, ms.Contains((ImGuiID)instance_id));
    }

    ms_inst_io = ImGui::EndMultiSelect();
    ms.ApplyRequests(ms_inst_io);
    ImGui::PopID();
 
    // update selection
    bool is_changed_selection = false;
    if (ms.is_changed) {
        m_volumes_ms.clear_all();

        if (!ImGui::GetIO().KeyCtrl) {
            m_instances_ms.clear_except(object_id);
            selected_items.clear();
        }

        for (const Slic3r::ModelInstance* instance : instances) {
            size_t instance_id = instance->id().id;
            if (instances_on_bed.count(instance_id)) {
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
    }

    return is_changed_selection;
}

void ObjectList::render_instance_node(const Slic3r::ModelObject* object, size_t inst_id, bool is_selected)
{
    const Slic3r::ModelInstance* instance = object->instances[inst_id];
    size_t id = instance->id().id;
    Domain::ElementRef sel_element{ object->id().id, id, 0 };

    const std::string name = icon_str(ImGui::ObjectIcon) + "Instance " + std::to_string(id);
  
    next_line();
    ImGui::SetNextItemSelectionUserData(inst_id);
    ImGui::Selectable(name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns);
    render_printable_icon(sel_element, instance->printable);

    handle_dragging(sel_element);
}

bool ObjectList::render_layer_ranges_node(const Slic3r::ModelObject* object)
{
    if (object->layer_config_ranges.empty())
        return false;

    next_line();
    const std::string name_id = "Ranges##obj_id" + std::to_string(object->id().id);
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    if (ImGui::TreeNodeEx(name_id.c_str(), node_flags | ImGuiTreeNodeFlags_Leaf, (icon_str(ImGui::HRModifier) + "Height range Modifier").c_str()))
        ImGui::TreePop();
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    Domain::ElementRef sel_element{ object->id().id };
    if (handle_selection(sel_element)) {
        force_select_whole_object(object);
        clear_all_ms();
        show_layer_ranges(sel_element);
        return true;
    }

    return false;
}

void ObjectList::render_infos_node(const Slic3r::ModelObject* object, bool is_sla_config)
{
    std::set<wchar_t> infos = get_infos(object, is_sla_config);
    if (infos.empty())
        return;

    std::string infos_line;
    for (const wchar_t icon : infos)
        infos_line += icon_str(icon);

    next_line();
    const std::string name_id = "Infos##obj_id" + std::to_string(object->id().id);
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    if (ImGui::TreeNodeEx(name_id.c_str(), node_flags | ImGuiTreeNodeFlags_Leaf, (infos_line + "Infos").c_str()))
        ImGui::TreePop();
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    render_infos_selectable(infos, object, hovered_current_row());
}

void ObjectList::render_edited(const char* init_name, const Domain::ElementRef& sel_element)
{
    static char buffer[128] = "";

    // put the editor item on the same line and without item spacing
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
    ImGui::SameLine();
    ImGui::PopStyleVar();
    // Editable text box
    strncpy(buffer, init_name, sizeof(buffer));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    if (ImGui::InputText("##edit", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        propagate_name_editing(sel_element, buffer);  // Save edited name
        m_edited_node_id = 0;  // Exit edit mode
    }
    ImGui::PopStyleVar(2);
}

void ObjectList::render_printable_icon(const Domain::ElementRef& sel_element, bool is_printable)
{
    if (hovered_current_row()) {
        if (icon_btn(ciPrintable, icon_str(is_printable ? ImGui::EyeOpen : ImGui::EyeClosed)))
            propagate_printable(sel_element, !is_printable);
    }
    else if (!is_printable)
        if (icon_btn(ciPrintable, icon_str(ImGui::EyeClosed)))
            propagate_printable(sel_element, !is_printable);
}

void ObjectList::render_overrides_icon(const Domain::ElementRef& sel_element, bool render)
{
    if (hovered_current_row()) {
        if (icon_btn(ciSettingsOvverides, icon_str(ImGui::PrintIconMarker)))
            show_overrides(sel_element);
    }
}

void ObjectList::render_extruder_marker(size_t extruder_id, const Domain::ElementRef& sel_element, bool is_bed/* = false*/)
{
    ImGui::TableSetColumnIndex(ciExtruder);
    const std::string extruder_name = std::to_string(extruder_id + 1);

    ImVec2 pos = ImGui::GetCursorScreenPos();

    const float size = ImGui::GetFontSize();
    const float radius = 0.5f * size;

    ImVec2 center = pos + ImVec2(radius, radius); // Offset from cursor
    ImGui::GetCurrentWindow()->DrawList->AddCircleFilled(center, radius, ImGui::GetColorU32(def_color));

    ImGui::SetCursorScreenPos(ImVec2(center.x - 0.5f * ImGui::CalcTextSize(extruder_name.c_str()).x, pos.y));
    ImGui::Text(extruder_name.c_str());

    if (ImGui::IsMouseHoveringRect(pos, pos + ImVec2(size, size)) && ImGui::IsMouseClicked(0))
        extruder_clicked(sel_element, is_bed);
}


static std::map<wchar_t, std::string> info_descriptions = {
    { ImGui::CustomSupports, "CustomSupports"} ,
    { ImGui::CustomSeam    , "Seam" },
    { ImGui::MmSegmentation, "MM Painting" },
    { ImGui::Sinking       , "Sinking" },
    { ImGui::FuzzySkin     , "Fuzzy Skin" },
};

void ObjectList::render_infos_selectable(const std::set<wchar_t>& infos, const Slic3r::ModelObject* object, bool force_render)
{
    static bool show{ false };
    if (force_render)
        show = true;

    if (show) {
        ImGui::SetNextWindowPos(ImVec2(3.f*ImGui::GetTreeNodeToLabelSpacing(), ImGui::GetMousePos().y - 10.f), ImGuiCond_Appearing);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.f, 5.f));
        ImGui::Begin("##infos_list", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

        bool any_hovered = false;
        for (wchar_t info : infos) {
            std::string line = icon_str(info) + info_descriptions[info];
            if (ImGui::Selectable(line.c_str())) {
                if (info == ImGui::Sinking) {
                    force_select_whole_object(object);
                    clear_all_ms();

                }

                show_gizmo({ object->id().id }, info);
                show = false;
            }
            any_hovered |= ImGui::IsItemHovered();
        }

        ImGui::End();
        ImGui::PopStyleVar(2);

        show = any_hovered;
    }
}

void ObjectList::clear_all_ms()
{
    m_instances_ms.clear_all();
    m_volumes_ms.clear_all();
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
    // ask project interactor to rename object/volume with id index
    m_scene_interactor->edit_name(id, new_name);
}

void ObjectList::propagate_printable(const Domain::ElementRef& sel_element, bool is_printable)
{
    // ask project interactor to change prinatble value for instance/object with id index
    m_scene_interactor->set_printable(sel_element, is_printable);
}

void ObjectList::show_overrides(const Domain::ElementRef& sel_element)
{
    test_out = Slic3r::format("propagate_show_overrides: obj%1%, inst%2%, vol%3%", 
                              sel_element.object_id, sel_element.instance_id, sel_element.volume_id);

    show_test_window(test_out);
}

void ObjectList::extruder_clicked(const Domain::ElementRef& sel_element, bool is_bed)
{
    if (is_bed)
        test_out = Slic3r::format("etruder_clicked: bed%1%", sel_element.object_id);
    else
        test_out = Slic3r::format("etruder_clicked: obj%1%, inst%2%, vol%3%",
            sel_element.object_id, sel_element.instance_id, sel_element.volume_id);
}

void ObjectList::show_layer_ranges(const Domain::ElementRef& sel_element)
{
    show_test_window(Slic3r::format("show_layer_ranges for obj%1%" , sel_element.object_id), true);
}

void ObjectList::show_gizmo(const Domain::ElementRef& sel_element, wchar_t gizmo_id)
{
    show_test_window(Slic3r::format("Open %1% %2% gizmo for object (id#%3%)", icon_str(gizmo_id), info_descriptions[gizmo_id], sel_element.object_id));
}

}// Slic3r::App::Plater namespace
