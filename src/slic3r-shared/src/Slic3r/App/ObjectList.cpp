#include "Slic3r/App/ObjectList.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Assert.hpp"

#include <libslic3r/Model.hpp>
#include "libslic3r/Color.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <boost/nowide/convert.hpp>
#include <vector>

//tmp include
#include "libslic3r/format.hpp"

namespace Slic3r::App {

enum ColumIndex
{
    ciTree = 0,
    ciPrintable,
    ciExtruder,

    ciCount
};

struct BoldFontGuard
{
    BoldFontGuard(Render::ImguiRender* imgui_render) {
        ImGui::PushFont(imgui_render->font(Render::ImguiFontType::Bold));
    }
    ~BoldFontGuard () {
        ImGui::PopFont();
    }
};

struct IndentGuard
{
    IndentGuard(float indent) {
        m_indent = indent;
        ImGui::Indent(m_indent);
    }
    ~IndentGuard() {
        ImGui::Unindent(m_indent);
    }
private:
    float m_indent;
};

struct BedsTable
{
    BedsTable() = default;
    ~BedsTable() {
        if (m_was_begin)
            ImGui::EndTable();
    }

    bool begin(size_t cc_id, ImGuiTableFlags m_table_flags) {
        const std::string cc_id_str = std::to_string(cc_id);
        m_was_begin = ImGui::BeginTable(("##BedsTable" + cc_id_str).c_str(), 3, m_table_flags);
        if (m_was_begin) {
            ImGui::TableSetupColumn(("##tree" + cc_id_str).c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(("##state" + cc_id_str).c_str(), ImGuiTableColumnFlags_WidthStretch, 0.2f);
            ImGui::TableSetupColumn(("##extruder" + cc_id_str).c_str(), ImGuiTableColumnFlags_WidthStretch, 0.15f);
        }
        return m_was_begin;
    }
private:
    bool m_was_begin{ false };
};

// color of the row background
static ImGuiCol_ row_color = ImGuiCol_COUNT;

/* Row background setter
 * predicate - condition for changing of the row color.
 * In constructor set color for "main-active item".
 * When call set_next(), than set color for "sub-active item".
 * Destructor invalidates row color
 */
struct RowBackground
{
    RowBackground(bool predicate) : force_apply(predicate) {
        if (force_apply)
            row_color = ImGuiCol_HeaderActive;
    }
    ~RowBackground() { invalidate(); }

    void set_next() {
        if (force_apply)
            row_color = ImGuiCol_Header;
    }

private:
    void invalidate() {
        if (force_apply)
            row_color = ImGuiCol_COUNT;
    }

    bool force_apply{ false };
};

/* Wrapper function to set cursor to the first cell of next row of the table
*/
static void new_row(float row_min_height = 0.0f)
{
    ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);
    if (row_color != ImGuiCol_COUNT)
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(row_color));
    ImGui::TableSetColumnIndex(0);
}

/* Selectable() ignores FramePagging, so for the rows with Selectable() items
 * we need to extend the CellPadding before set cursor on new row.
 * Revertion of the CellPadding have to be processed at the and of row,
 * that is why struct is used in this case
 */
struct NewRowWithSelectable
{
    NewRowWithSelectable() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GImGui->Style.CellPadding.x, GImGui->Style.FramePadding.y));
        new_row();
    }
    ~NewRowWithSelectable() {
        ImGui::PopStyleVar();
    }
};

static std::string icon_str(const wchar_t icon)
{
    return Slic3r::format(" %1%  ", boost::nowide::narrow(std::wstring(&icon, 1)));
}

static std::string icon_str(const Slic3r::ModelVolume* volume)
{
    if (volume->is_text()) {
        switch (volume->type()) {
        case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::TextSolidPartVolume);
        case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::TextNegativeVolume);
        case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::TextModifierVolume);
        }
        return "";
    }
    if (volume->is_svg()) {
        switch (volume->type()) {
        case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::SvgSolidPartVolume);
        case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::SvgNegativeVolume);
        case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::SvgModifierVolume);
        }
        return "";
    }

    switch (volume->type()) {
    case Slic3r::ModelVolumeType::MODEL_PART        : return icon_str(ImGui::SolidPartVolume);
    case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return icon_str(ImGui::NegativeVolume );
    case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return icon_str(ImGui::ModifierVolume );
    case Slic3r::ModelVolumeType::SUPPORT_BLOCKER   : return icon_str(ImGui::SupportBlocker );
    case Slic3r::ModelVolumeType::SUPPORT_ENFORCER  : return icon_str(ImGui::SupportModifier);
    default:
        return "";
    }
}

static std::string volume_icon_tooltip(const Slic3r::ModelVolume* volume)
{
    switch (volume->type()) {
    case Slic3r::ModelVolumeType::MODEL_PART        : return "Solid Part Volume ";
    case Slic3r::ModelVolumeType::NEGATIVE_VOLUME   : return "Negative Volume ";
    case Slic3r::ModelVolumeType::PARAMETER_MODIFIER: return "Modifier Volume  ";
    case Slic3r::ModelVolumeType::SUPPORT_BLOCKER   : return "Support Blocker ";
    case Slic3r::ModelVolumeType::SUPPORT_ENFORCER  : return "Support Modifier";
    default:
        return "";
    }
}

static std::string get_cc_name(const Slic3r::DynamicPrintConfig& print_config)
{
    return icon_str(ImGui::ConfigContainer) + print_config.opt_string("printer_model");

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

static std::set<wchar_t> get_infos(const Slic3r::ModelObject* object, bool is_sla_config)
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
        if (!object->layer_config_ranges.empty())
            infos.insert(ImGui::HRModifier);
        //if (wxGetApp().plater()->canvas3D()->is_object_sinking(obj_idx))
        //    infos.insert(ImGui::Sinking);
    }

    return infos;
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

static void toggle_icon_btn(const wchar_t icon, bool* is_toggled, const std::string id, ColumIndex ci = ColumIndex::ciCount/*undef*/)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32((*is_toggled) ? ImGuiCol_ButtonActive : ImGuiCol_Button));
    if (Imgui::icon_button(icon, ImVec2(), id))
        *is_toggled = !*is_toggled; // Toggle state
    ImGui::PopStyleColor();
}

// object is simple: has just one instance, one volume and no aditional information
static bool has_overrides(const Slic3r::ModelObject* object, bool is_sla_config)
{
    bool has_config_overrides = !object->config.empty() || !object->layer_config_ranges.empty();
    if (!has_config_overrides) {
        for (auto volume : object->volumes)
            if (!volume->config.empty()) {
                has_config_overrides = true;
                break;
            }
    }
    return has_config_overrides || !get_infos(object, is_sla_config).empty();
}

// object is simple: has just one instance, one volume and no aditional information
static bool is_simple(const Slic3r::ModelObject* object, bool is_sla_config)
{
    return  object->instances.size() == 1 &&
            object->volumes.size() == 1 &&
            !has_overrides(object, is_sla_config);
}

static bool is_imgui_item_just_selected()
{
    // handle selection only on MouseRelease or PressEnter
    return ImGui::IsItemHovered(ImGuiHoveredFlags_None) &&
        (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter));
}

// hendle selection from the tree nodes
bool ObjectList::handle_selection(const Domain::ElementRef& id)
{
    // handle selection only when we are NOT dragging and on MouseRelease or PressEnter
    if (!m_is_dragging && is_imgui_item_just_selected()) {
        if (ImGui::GetIO().KeyCtrl) {
            if (m_selected_items.count(id))
                m_selected_items.erase(id);  // Toggle deselect
            else
                m_selected_items.insert(id); // Multi-select
        }
        else {
            m_selected_items.clear();
            m_selected_items.insert(id);     // Single-select
        }
        return true;
    }
    return false;
}

void ObjectList::handle_dragging(const Domain::ElementRef& id)
{
    if (m_scene_map)
        return;

    // Detect dragging on any selected node
    if (m_selected_items.count(id) && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        m_is_dragging = true;
}

void ObjectList::force_select_whole_object(const Slic3r::ModelObject* object)
{
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };
    if (m_selected_items.count(sel_element)) {
        // remove object element
        m_selected_items.erase(sel_element);
        //  and push all instances instad
        for (const Slic3r::ModelInstance* instance : object->instances)
            m_selected_items.insert(Domain::ElementRef{ object_id, instance->id().id });
    }
}

using namespace Imgui;

void ObjectList::bold_text(const std::string& text)
{
    BoldFontGuard bfg(m_imgui_render);
    ImGui::Text(text.c_str());
}

ObjectList::ObjectList() : Window("object_list") {
    set_min_size({330.f, 0.f});
}

void ObjectList::init(Biz::ProjectInteractor *project_interactor, Mode mode)
{
    m_project_interactor = project_interactor;
    m_mode = mode;
}

void ObjectList::render_body(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    m_scene_interactor = &m_project_interactor->scene_interactor();
    m_model            = &m_project_interactor->selected_project().model();
    assert(m_model && m_scene_interactor);

    invalidate_bed_selection();
    update_selection_from_scene();

    setup_ui_state();
    render_header(pos, size);

    const float slised_info_height{ 120.f };

    // Define a region for the tree/scene_map control
    size.y() -= (ImGui::GetCursorScreenPos().y - pos.y()) + 
                (m_mode == Mode::Preview ? slised_info_height : GImGui->Style.WindowRounding);

    if (m_scene_map) {
        render_scene_map(size);
    }
    else {
        if (render_list(size)) {
            // update selection on the scene
            propagate_selection();
        }
        process_dragging_start();
    }

    render_sliced_info(slised_info_height);
}

void ObjectList::setup_ui_state()
{
    m_inner_padding = Vec2f(GImGui->FontSize, 1.25f * GImGui->FontSize);
    m_multi_selection_flags =   ImGuiMultiSelectFlags_ScopeRect |
                                ImGuiMultiSelectFlags_ClearOnEscape |
                                ImGuiMultiSelectFlags_BoxSelect1d |
                                ImGuiMultiSelectFlags_SelectOnClick;

    m_node_flags =  ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_FramePadding |
                    ImGuiTreeNodeFlags_SpanAllColumns;

    m_table_flags = ImGuiTableFlags_NoBordersInBody |
             //       ImGuiTableFlags_Borders | 
                    ImGuiTableFlags_NoPadInnerX;

    m_is_dragging = false;
}

void ObjectList::process_dragging_start()
{
    // Start drag operation when any selected node is being dragged
    if (m_is_dragging && ImGui::BeginDragDropSource(/*ImGuiDragDropFlags_SourceNoHoldToOpenOthers | */ImGuiDragDropFlags_SourceExtern)) {
        int size = (int)m_selected_items.size();
        assert(size > 0);
        if (m_selected_items.begin()->has_volume()) {
            ImGui::Text("Mixing volume(s)");
            ImGui::SetDragDropPayload("MULTI_VOLUMES", &size, sizeof(int));
        }
        else {
            ImGui::Text("Extracting instances into separate object");
            ImGui::SetDragDropPayload("MULTI_INSTANCES", &size, sizeof(int));
        }
        ImGui::EndDragDropSource();
    }
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

    std::set<Domain::ElementRef> selectedItems_tmp = std::set<Domain::ElementRef>(scene_selection.elements.begin(), scene_selection.elements.end());
    if (selectedItems_tmp != m_selected_items) {
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
        m_selected_items = selectedItems_tmp;
    }
}

bool ObjectList::render_list(Domain::Vec2f size)
{
    // ysFIXME delete after new layout apply!!!
    // Temporary fix for the assert in Debug mode
    if (GImGui->CurrentWindow && std::string(GImGui->CurrentWindow->Name) == "Debug##Default")
        return false;

    ImGui::BeginChild("ObjectList", ImVec2(-FLT_MIN, size.y()));

    bool is_changed_selection = render_config_containers();
    is_changed_selection |= render_out_of_beds();

    render_drop_target_area();

    ImGui::EndChild();

    return is_changed_selection;
}

void ObjectList::render_header(Domain::Vec2f pos, Domain::Vec2f size)
{
    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(m_inner_padding.x(), m_inner_padding.y()));
    bold_text(m_scene_map ? L("Scene map") : L("Objects"));

    float btn_width = 2 * ImGui::GetFontSize();
    float btn_pos = size.x() - pos.x() - btn_width;
    ImGui::SameLine(btn_pos);

    if (m_mode == Mode::Plater) {
        toggle_icon_btn(ImGui::Details, &m_show_details, "details");
        Imgui::item_tooltip("Show object details");

        btn_pos -= pos.x() + btn_width;
        ImGui::SameLine(btn_pos);
        if (Imgui::icon_button(ImGui::AddBedIcon, ImVec2(), "add_bed")) {
            // add bed
            m_scene_interactor->add_bed_instance(m_project_interactor->selected_config_container().id().id);
        }
        Imgui::item_tooltip("Add bed");
    }
    else {
        toggle_icon_btn(ImGui::SceneMap, &m_scene_map, "scene_map");
        Imgui::item_tooltip("Show scene map");
    }
}

bool ObjectList::tree_node(const char* str_id, ImGuiTreeNodeFlags flags, const std::string& label, bool add_overrides_marker/* = false*/)
{
    // get initial cursor position
    ImVec2 pos_old = ImGui::GetCursorScreenPos();
    // render node as it is
    bool is_open = ImGui::TreeNodeEx(str_id, flags, label.c_str());

    // for leaf node no need to redrow of arrows
    if ((flags & ImGuiTreeNodeFlags_Leaf) != 0)
        return is_open;

    // get cursor position for the end of tree node rendering
    ImVec2 pos_new = ImGui::GetCursorScreenPos();

    const bool is_selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 padding = ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    const float text_offset_x = g.FontSize + padding.x * 2;                             // Collapsing arrow width + Spacing
    const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);    // Latch before ItemSize changes it

    ImVec2 text_pos(pos_old.x + text_offset_x, pos_old.y + text_offset_y + style.FramePadding.y);

    ImVec2 pos = ImVec2(text_pos.x - text_offset_x, text_pos.y);
    ImVec2 pos_end = pos + ImVec2(g.FontSize, g.FontSize);

    // render rect over the triangle
    ImDrawList* draw_list = ImGui::GetCurrentWindow()->DrawList;

    ImGuiID active_id = ImGui::GetActiveID();

    // first layer
    draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_WindowBg));
    if (row_color != ImGuiCol_COUNT)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(row_color));

    ImGuiID edit_name_input_id = ImGui::GetID("##edit");

    //second layer
    if (ImGui::IsItemActive() && ImGui::IsItemHovered())
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_HeaderActive));
    else if (ImGui::IsItemHovered() && !m_is_edit_name_input_hovered)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
    else if (is_selected)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_Header));

    // render open-close new arrow
    draw_list->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), boost::nowide::narrow(std::wstring(&(is_open ? ImGui::OpenArrow : ImGui::CloseArrow), 1)).c_str());

    if (add_overrides_marker)
        draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), icon_str(ImGui::OverridesMarker).c_str());

    // revert cursore position
    ImGui::SetCursorScreenPos(pos_new);
    return is_open;
}

static bool selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns, bool add_overrides_marker = false)
{
    ImVec2 init_pos(ImGui::GetCursorScreenPos());

    ImGuiStyle style = GImGui->Style;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 2 * style.FramePadding.y));
    bool ret = ImGui::Selectable(label, selected, flags);
    ImGui::PopStyleVar();

    if (add_overrides_marker) {
        ImGui::SameLine(init_pos.x - style.ItemSpacing.x - style.CellPadding.x);
        ImGui::Text(icon_str(ImGui::OverridesMarker).c_str());
        ImGui::SameLine();
    }

    return ret;
}

bool ObjectList::render_config_containers()
{
    bool is_changed_selection = false;

    size_t beds_cnt{ 0 };
    for (auto& cc : m_scene_interactor->selected_project_config_containers()) {
        render_group_name(get_cc_name(cc->print_config()));

        BedsTable table;
        if (table.begin(cc->id().id, m_table_flags)) {
            IndentGuard ig(m_inner_padding.x());

            for (auto& bed_inst : cc->bed_instances()) {
                if (!bed_inst->model_instances.empty())
                    beds_cnt++;
                is_changed_selection |= render_bed_node(bed_inst.get(), cc->id().id);
            }

            // check bed selection
            if (m_selected_bed_instance_id != 0) {
                m_selected_container_id = cc->id().id;
            }
        }        
    }

    if (beds_cnt > 1 && m_mode == Mode::Preview)
        render_all_beds_node();

    return is_changed_selection;
}

void ObjectList::render_group_name(const std::string& name)
{
    IndentGuard ig(m_inner_padding.x());
    BoldFontGuard bfg(m_imgui_render);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, m_inner_padding.y() * 0.25f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.25f * m_inner_padding.y()));
    ImGui::Text(name.c_str());
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ObjectList::render_all_beds_node()
{
    size_t total_beds_cnt = 0;
    size_t finished_beds_cnt = 0;
    for (auto& cc : m_scene_interactor->selected_project_config_containers()) {
        for (auto& bed_inst : cc->bed_instances()) {
            if (bed_inst->model_instances.empty())
                continue;
            total_beds_cnt++;
            const std::optional<Biz::Slicing::Status> status{
            m_project_interactor->status_cache().get_status({ m_project_interactor->selected_project_id(), bed_inst->id().id})};
            if (status && status == Biz::Slicing::Status::Finished)
                finished_beds_cnt++;
        }
    }
    DEBUG_ASSERT(total_beds_cnt != 0);

    ImVec2 progress_bar_sz(75.f, ImGui::GetFontSize() + 4.f);

    if (ImGui::BeginTable("##AllBeds", 2, m_table_flags)) {
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##progress", ImGuiTableColumnFlags_WidthStretch, 0.35f);

        ImGui::TableNextColumn();
        render_group_name(icon_str(ImGui::AllBeds) + " All");

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.25f * m_inner_padding.y()));

        BoldFontGuard bfg(m_imgui_render);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetColorU32(BLUE_BUTTON_COLOR));
        if (total_beds_cnt == finished_beds_cnt)
            ImGui::ProgressBar(1.0, progress_bar_sz, "SLICED");
        else
            ImGui::ProgressBar(float(finished_beds_cnt) / float(total_beds_cnt), progress_bar_sz, Slic3r::format("%1%/%2% SLICED", finished_beds_cnt, total_beds_cnt).c_str());
        ImGui::PopStyleColor();

        ImGui::EndTable();
    }
}

bool ObjectList::render_out_of_beds()
{
    if (m_mode == Mode::Preview ||
        m_scene_interactor->selected_project_unplaced_model_instances().empty())
        return false;

    bool is_changed_selection = false;

    render_group_name(L("Out of bed"));

    BedsTable table;
    if (table.begin(size_t(-1), m_table_flags)) {
        IndentGuard ig(m_inner_padding.x());
        for (const Slic3r::ModelObject* object : m_model->objects) {
            if (bed_has_object(m_scene_interactor->selected_project_unplaced_model_instances(), object))
                is_changed_selection |= render_object_node(object);
        }
    }

    return is_changed_selection;
}

void ObjectList::render_drop_target_area()
{
    if (m_mode != Mode::Plater)
        return;

    // Make the entire window a valid drop target
    const float drop_area_height = 50.f;
    ImGui::InvisibleButton("InstancesDropZone", ImVec2(ImGui::GetContentRegionAvail().x, drop_area_height));  // Creates an invisible instances drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MULTI_INSTANCES")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            ask_extract_selected_instances();
            m_selected_items.clear();  // Clear selection after drop
        }
        ImGui::EndDragDropTarget();
    }
}

bool ObjectList::render_bed_node(const Domain::BedInstance* bed, size_t config_container_id)
{
    size_t bed_id = bed->id().id;
    bool is_sla_config = m_project_interactor->selected_project().find_config_container(config_container_id)->print_technology() == ptSLA;

    const std::string name = "Bed " + std::to_string(bed_id);
    const std::string name_id = "##bed_id" + std::to_string(bed_id);

    const std::string name_with_icon = Slic3r::format("%1%%2%", boost::nowide::narrow(std::wstring(&ImGui::BedThumbnail, 1)), name);

    const ImVec2 icon_size = ImVec2(40.f, 40.f);
    const ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
    const ImVec2 padding(5.f, 5.f);

    RowBackground bg(bed->active && m_selected_items.empty());
    new_row(icon_size.y + 2.f*padding.y);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding.y, 0.5f * (icon_size.y - text_size.y) + padding.y));
    bool is_open = tree_node(name_id.c_str(), m_node_flags | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap, name_with_icon);
    ImGui::PopStyleVar();

    bool is_changed_selection = false;
    // check bed selection
    if (!m_is_dragging && is_imgui_item_just_selected()) {
        m_selected_bed_instance_id = bed->id().id;
        is_changed_selection = true;
    }

    render_slicing_state_marker(bed_id);
    if (!is_sla_config)
        render_extruder_marker(0, { "#E74840" });

    if (is_open) {
        bg.set_next();
        for (const Slic3r::ModelObject* object : m_model->objects) {
            if (bed_has_object(bed->model_instances, object))
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

    ImGuiTreeNodeFlags flags = m_node_flags;
    if (is_selected)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (is_simple(object, is_sla_config))
        flags |= ImGuiTreeNodeFlags_Leaf;

    const std::string name = (object->name.empty() ? "Object " + std::to_string(object_id) : object->name);
    const std::string name_id = "##obj_id" + std::to_string(object_id);

    RowBackground bg(is_selected);
    new_row();

    bool isOpen = false;
    if (m_edited_node_id == object_id && is_selected) {
        isOpen = tree_node(name_id.c_str(), flags | ImGuiTreeNodeFlags_AllowOverlap, icon_str(ImGui::ObjectIcon), has_overrides(object, is_sla_config));
        render_edited(name.c_str(), { object_id });
    }
    else
        isOpen = tree_node(name_id.c_str(), flags | ImGuiTreeNodeFlags_AllowOverlap, (icon_str(ImGui::ObjectIcon) + name), has_overrides(object, is_sla_config));

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
    if (!is_sla_config)
        render_extruder_marker(1, { "#240E74", "#E74840", "#FAD73B" });

    if (isOpen) {
        bg.set_next();
        is_changed_selection |= render_connectors_node(object, bed ? bed->id().id : 0);
        is_changed_selection |= render_volumes(object, bed ? bed->id().id : 0, is_sla_config);
        is_changed_selection |= render_instances_node(object, bed);
        if (m_show_details)
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

    new_row();
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    if (tree_node(name_id.c_str(), m_node_flags | ImGuiTreeNodeFlags_Leaf, icon_str(ImGui::CutConnectors) + "Connectors"))
        ImGui::TreePop();
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    if (handle_selection({ object_id, instance_id })) {
        clear_all_ms();

        std::set<Domain::ElementRef> selectedItems_tmp;
        for (const Slic3r::ModelVolume* volume : object->volumes) {
            if (volume->is_cut_connector())
                selectedItems_tmp.insert({ object_id, instance_id, volume->id().id });
        }
        if (m_selected_items != selectedItems_tmp) {
            m_selected_items = selectedItems_tmp;
            return true;
        }
    }

    return false;
}

bool ObjectList::render_volumes(const Slic3r::ModelObject* object, size_t bed_id, bool is_sla_config)
{
    if (visible_volumes_count(object) < 2)
        return false;

    size_t object_id = object->id().id;
    size_t instance_id = object->instances[0]->id().id;

    const std::string name_id = "##volumes_id" + std::to_string(bed_id) + std::to_string(object_id);

    new_row();
    bool is_open = tree_node(name_id.c_str(), m_node_flags, "Volumes");

    const Slic3r::ModelVolumePtrs& volumes = object->volumes;
    MultiSelectionStorage& ms = m_volumes_ms.at(object_id);

    if (is_open) {
        ImGui::PushID(&ms);  // Ensure unique ID
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(m_multi_selection_flags, ms.Size, visible_volumes_count(object));
        ms.ApplyRequests(ms_io);

        for (size_t vol_id = 0; vol_id < volumes.size(); vol_id++) {
            const Slic3r::ModelVolume* volume = object->volumes[vol_id];
            if (volume->is_cut_connector())
                continue;
            size_t volume_id = volume->id().id;
            ImGui::SetNextItemSelectionUserData(vol_id);
            render_volume_node(volume, { object_id, instance_id, volume_id }, ms.Contains((ImGuiID)volume_id), is_sla_config);
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

        if (m_selected_items != selectedItems_tmp) {
            m_selected_items = selectedItems_tmp;
            return true;
        }
    }

    return false;
}

// render edited item as an input text and propagate new name to scene_interactor
void ObjectList::render_volume_node(const Slic3r::ModelVolume* volume, const Domain::ElementRef& sel_element, bool is_selected, bool is_sla_config)
{
    std::string volume_name = (volume->name.empty() ? "Volume " + std::to_string(volume->id().id) : volume->name);
    size_t volume_id = volume->id().id;

    if (m_edited_node_id == volume_id && !is_selected)
        m_edited_node_id = 0;  // Exit edit mode

    NewRowWithSelectable row;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool has_config_overrides = !volume->config.empty();
    bool has_extruder_overrides = !is_sla_config && has_config_overrides && volume->config.has("extruder");

    if (m_edited_node_id == volume_id) {
        if (selectable(icon_str(volume).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, has_config_overrides)) {
            m_edited_node_id = 0;// Discard edit mode on selection
        }
        render_edited(volume_name.c_str(), sel_element);

        // During rendering of the InputText different values of cursore position are changed.
        // As a result positions of other items in the row are missed up.
        // But invalidating of the RowCellPaddingY fixes this issue
        GImGui->CurrentTable->RowCellPaddingY = 0.f;
    }
    else {
        // Display as a selectable label
        if (selectable((icon_str(volume) + volume_name).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns, has_config_overrides))
            m_edited_node_id = volume_id;  // Start edit mode on selection
    }

    const ImVec2 size = ImGui::CalcTextSize(icon_str(volume).c_str());
    if (ImGui::IsMouseHoveringRect(pos, pos + size))
        Imgui::tooltip(volume_icon_tooltip(volume));

    if (has_extruder_overrides)
        render_extruder_marker(2, {"#40E740"});
}

bool ObjectList::render_instances_node(const Slic3r::ModelObject* object, const Domain::BedInstance* bed /*= nullptr*/)
{
    if (object->instances.size() == 1)
        return false;
    std::set<size_t> instances_on_bed = get_object_instance_ids_on_bed(bed ? bed->model_instances : m_scene_interactor->selected_project_unplaced_model_instances(), object);
    if (instances_on_bed.empty())
        return false;

    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{ object_id };

    MultiSelectionStorage& ms = m_instances_ms.at(object_id);

    const std::string name_id = "Instances##obj_id" + std::to_string(object_id);

    new_row();
    bool isOpen = tree_node(name_id.c_str(), m_node_flags | ImGuiTreeNodeFlags_DefaultOpen, icon_str(ImGui::InstancesIcon) + "Instances");

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
    ImGuiMultiSelectIO* ms_inst_io = ImGui::BeginMultiSelect(m_multi_selection_flags, ms.Size, instances_on_bed.size());
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
            m_selected_items.clear();
        }

        for (const Slic3r::ModelInstance* instance : instances) {
            size_t instance_id = instance->id().id;
            if (instances_on_bed.count(instance_id)) {
                Domain::ElementRef sel_element{ object_id, instance->id().id };

                if (ms.Contains((ImGuiID)instance_id) && !m_selected_items.count(sel_element)) {
                    is_changed_selection = true;
                    m_selected_items.insert(sel_element);
                }
                else if (!ms.Contains((ImGuiID)instance_id) && m_selected_items.count(sel_element)) {
                    is_changed_selection = true;
                    m_selected_items.erase(sel_element);
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

    NewRowWithSelectable row;
    ImGui::SetNextItemSelectionUserData(inst_id);
    selectable(name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

    render_printable_icon(sel_element, instance->printable);

    handle_dragging(sel_element);
}

void ObjectList::render_infos_node(const Slic3r::ModelObject* object, bool is_sla_config)
{
    std::set<wchar_t> infos = get_infos(object, is_sla_config);
    if (infos.empty())
        return;

    new_row();
    const std::string name_id = "Infos##obj_id" + std::to_string(object->id().id);
    if (tree_node(name_id.c_str(), m_node_flags, "Infos")) {
        render_infos_selectable(infos, object, hovered_current_row());
        ImGui::TreePop();
    }
}

void ObjectList::render_edited(const char* init_name, const Domain::ElementRef& sel_element)
{
    static char buffer[128] = "";
    strncpy(buffer, init_name, sizeof(buffer));

    // put the editor item on the same line and without item spacing
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
    ImGui::SameLine();
    // modify cursore position in respect to the cell padding
    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() - ImVec2(0.f, GImGui->Style.CellPadding.y));
    // discard horizontal frame padding
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, GImGui->Style.FramePadding.y));

    // Editable text box
    if (ImGui::InputText("##edit", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        propagate_name_editing(sel_element, buffer);  // Save edited name
        m_edited_node_id = 0;  // Exit edit mode
    }
    //save hovered state for InputText => will be used in tree_node rendering
    m_is_edit_name_input_hovered = ImGui::IsItemHovered();

    ImGui::PopStyleVar(2);
}

void ObjectList::render_printable_icon(const Domain::ElementRef& sel_id, bool is_printable)
{
    wchar_t icon{ L'\0' };
    if (hovered_current_row()) {
        icon = is_printable ? ImGui::EyeOpen : ImGui::EyeClosed;
    }
    else if (!is_printable) {
        icon = ImGui::EyeClosed;
    }
    else
        return;

    ImGui::TableSetColumnIndex(ciPrintable);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_Button));
    ImGui::PushID(Slic3r::format("##print_%1%_%2%_%3%", sel_id.object_id, sel_id.instance_id, sel_id.volume_id).c_str());  // Ensure unique ID
    if (button_aligned(1.f, boost::nowide::narrow(std::wstring(&icon, 1)), ImVec2(), ImGuiButtonFlags_AlignTextBaseLine))
        propagate_printable(sel_id, !is_printable);
    ImGui::PopID();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ObjectList::render_extruder_marker(size_t extruder_id, const std::vector<std::string>& str_colors)
{
    ImGui::TableSetColumnIndex(ciExtruder);
    BoldFontGuard bfg(m_imgui_render);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_WindowBg));

    std::vector<ImVec4> colors;
    colors.reserve(str_colors.size());
    ColorRGB clr;
    for (const std::string& str_color : str_colors) {
        DEBUG_ASSERT(can_decode_color(str_color));
        decode_color(str_color, clr);
        colors.push_back({ clr.r(), clr.g(), clr.b(), 1.f });
    }

    colored_circle_marker_aligned(0.5f, str_colors.size() == 1 ? std::to_string(extruder_id) : "", colors);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ObjectList::render_slicing_state_marker(size_t bed_instance_id)
{
    const std::optional<Biz::Slicing::Status> status{ 
        m_project_interactor->status_cache().get_status({ m_project_interactor->selected_project_id(), bed_instance_id }) };
    if (!status || status == Biz::Slicing::Status::Empty)
        return;

    const float align_x{ 1.f };

    ImGui::TableSetColumnIndex(ciPrintable);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 1.f));
    if (status == Biz::Slicing::Status::Finished) {
        BoldFontGuard bfg(m_imgui_render);
        text_with_bg_aligned(align_x, L("SLICED"), BLUE_BUTTON_COLOR);
    }
    else if (status != Biz::Slicing::Status::Modified) {
        text_with_bg_aligned(align_x, L("SLICING"), { 0.32f, 0.48f, 0.84f, 0.65f });
    }
    else if (m_mode == Mode::Preview) {
        BoldFontGuard bfg(m_imgui_render);
        ImGui::PushStyleColor(ImGuiCol_Button, ORANGE_BUTTON_COLOR);
        if (button_aligned(align_x, L("SLICE"), ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine))
            m_project_interactor->slicing_interactor().slice_bed(bed_instance_id);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
}


static std::map<wchar_t, std::string> info_descriptions = {
    { ImGui::CustomSupports, "CustomSupports"} ,
    { ImGui::CustomSeam    , "Seam" },
    { ImGui::MmSegmentation, "MM Painting" },
    { ImGui::Sinking       , "Sinking" },
    { ImGui::FuzzySkin     , "Fuzzy Skin" },
    { ImGui::HRModifier    , "Height range Modifier" },
};

void ObjectList::render_infos_selectable(const std::set<wchar_t>& infos, const Slic3r::ModelObject* object, bool force_render)
{
    for (wchar_t info : infos) {
        NewRowWithSelectable row;
        std::string line = icon_str(info) + info_descriptions[info];
        if (selectable(line.c_str())) {
            if (info == ImGui::Sinking || info == ImGui::HRModifier) {
                force_select_whole_object(object);
                clear_all_ms();
            }
            else
                show_gizmo({ object->id().id }, info);
        }
    }
}

void ObjectList::clear_all_ms()
{
    m_instances_ms.clear_all();
    m_volumes_ms.clear_all();
}

void ObjectList::invalidate_bed_selection()
{
    m_selected_container_id = 0;
    m_selected_bed_instance_id = 0;
}

void ObjectList::propagate_selection()
{
    if (m_selected_bed_instance_id != 0) {
        m_scene_interactor->select_bed_instance({ m_selected_container_id, m_selected_bed_instance_id });
        return;
    }

    Biz::Scene::Selection sels;
    sels.elements = std::vector<Domain::ElementRef>(m_selected_items.begin(), m_selected_items.end());
    sels.normalize();
    m_scene_interactor->set_selection(sels);
}

void ObjectList::propagate_name_editing(const Domain::ElementRef& id, const std::string& new_name)
{
    // ask project interactor to rename object/volume with id index
    m_scene_interactor->edit_name(id, new_name);
}

void ObjectList::propagate_printable(const Domain::ElementRef& id, bool is_printable)
{
    m_scene_interactor->set_printable(id, is_printable);
}

void ObjectList::ask_extract_selected_instances()
{
    m_scene_interactor->extract_selected_instances();
}

void ObjectList::extruder_clicked(const Domain::ElementRef& sel_element, bool is_bed)
{
    // ToDo
}

void ObjectList::show_layer_ranges(const Domain::ElementRef& sel_element)
{
    // ToDo
}

void ObjectList::show_gizmo(const Domain::ElementRef& sel_element, wchar_t gizmo_id)
{
    // ToDo
}

void ObjectList::render_scene_map(Domain::Vec2f size)
{
    ImGui::BeginChild("SceneMap", { size.x(), size.y() }, ImGuiChildFlags_FrameStyle);

    static bool isometric_view{ true };
    toggle_button(L("Isometric view"), &isometric_view);

    ImGui::EndChild();
}

void ObjectList::render_sliced_info(float height)
{
    if (m_mode != Mode::Preview)
        return;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(m_inner_padding.x(), m_inner_padding.y()));

    ImGui::BeginChild("SlicedInfo", ImVec2(-FLT_MIN, height), ImGuiChildFlags_FrameStyle);

    bold_text(L("Sliced Info"));

    // ysFIXME delete after new layout apply!!!
    // Temporary fix for the assert in Debug mode
    if (GImGui->CurrentWindow && std::string(GImGui->CurrentWindow->Name) == "Debug##Default")
        return;

    ImGui::Dummy(ImVec2(0, 0.5f * m_inner_padding.y()));

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 5.f));
    if (ImGui::BeginTable("##SlicedInfoTable", 2, m_table_flags, ImVec2())) {
        ImGui::TableSetupColumn("##desriprion", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

        new_row();
        bold_text(L("Used material"));
        ImGui::TableNextColumn();
        ImGui::Text(L("data in g, m, mm3").c_str());

        new_row();
        bold_text(L("Printing time"));
        ImGui::TableNextColumn();
        ImGui::Text(L("data").c_str());

        ImGui::EndTable();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

}// Slic3r::App namespace
