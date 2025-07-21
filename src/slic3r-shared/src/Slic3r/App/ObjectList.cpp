#include "Slic3r/App/ObjectList.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Plater/BedThumbnailTexture.hpp"

#include "Slic3r/Assert.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

#include <boost/nowide/convert.hpp>
#include <vector>

// tmp include
#include "libslic3r/format.hpp"

using Slic3r::Domain::ColorRGB;
using Slic3r::Domain::Vec2f;

using Slic3r::Biz::Algorithms::Color::can_decode_color;
using Slic3r::Biz::Algorithms::Color::decode_color;

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
    BoldFontGuard(Render::ImguiRender* imgui_render)
    {
        ImGui::PushFont(imgui_render->font(Render::ImguiFontType::Bold));
    }

    ~BoldFontGuard()
    {
        ImGui::PopFont();
    }
};

struct IndentGuard
{
    IndentGuard(float indent)
    {
        m_indent = indent;
        ImGui::Indent(m_indent);
    }

    ~IndentGuard()
    {
        ImGui::Unindent(m_indent);
    }

private:
    float m_indent;
};

struct BedsTable
{
    BedsTable() = default;

    ~BedsTable()
    {
        if (m_was_begin)
            ImGui::EndTable();
    }

    bool begin(size_t cc_id, ImGuiTableFlags m_table_flags)
    {
        const std::string cc_id_str = std::to_string(cc_id);
        m_was_begin = ImGui::BeginTable(("##BedsTable" + cc_id_str).c_str(), 3, m_table_flags);
        if (m_was_begin) {
            ImGui::TableSetupColumn(("##tree" + cc_id_str).c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(
                ("##state" + cc_id_str).c_str(),
                ImGuiTableColumnFlags_WidthStretch,
                0.2f
            );
            ImGui::TableSetupColumn(
                ("##extruder" + cc_id_str).c_str(),
                ImGuiTableColumnFlags_WidthStretch,
                0.15f
            );
        }
        return m_was_begin;
    }

private:
    bool m_was_begin{false};
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
    RowBackground(bool predicate) : force_apply(predicate)
    {
        if (force_apply)
            row_color = ImGuiCol_HeaderActive;
    }

    ~RowBackground()
    {
        invalidate();
    }

    void set_next()
    {
        if (force_apply)
            row_color = ImGuiCol_Header;
    }

private:
    void invalidate()
    {
        if (force_apply)
            row_color = ImGuiCol_COUNT;
    }

    bool force_apply{false};
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
    NewRowWithSelectable()
    {
        ImGui::PushStyleVar(
            ImGuiStyleVar_CellPadding,
            ImVec2(GImGui->Style.CellPadding.x, GImGui->Style.FramePadding.y)
        );
        new_row();
    }

    ~NewRowWithSelectable()
    {
        ImGui::PopStyleVar();
    }
};

static std::string icon_str(const Render::Icon icon)
{
    wchar_t icon_char = static_cast<wchar_t>(icon);
    return Slic3r::format(" %1%  ", boost::nowide::narrow(std::wstring(&icon_char, 1)));
}

static std::string icon_str(const Domain::ModelVolume* volume)
{
    if (volume->is_text()) {
        switch (volume->type()) {
        case Domain::ModelVolumeType::MODEL_PART:
            return icon_str(Render::Icon::TextSolidPartVolume);
        case Domain::ModelVolumeType::NEGATIVE_VOLUME:
            return icon_str(Render::Icon::TextNegativeVolume);
        case Domain::ModelVolumeType::PARAMETER_MODIFIER:
            return icon_str(Render::Icon::TextModifierVolume);
        }
        return "";
    }
    if (volume->is_svg()) {
        switch (volume->type()) {
        case Domain::ModelVolumeType::MODEL_PART:
            return icon_str(Render::Icon::SvgSolidPartVolume);
        case Domain::ModelVolumeType::NEGATIVE_VOLUME:
            return icon_str(Render::Icon::SvgNegativeVolume);
        case Domain::ModelVolumeType::PARAMETER_MODIFIER:
            return icon_str(Render::Icon::SvgModifierVolume);
        }
        return "";
    }

    switch (volume->type()) {
    case Domain::ModelVolumeType::MODEL_PART:
        return icon_str(Render::Icon::SolidPartVolume);
    case Domain::ModelVolumeType::NEGATIVE_VOLUME:
        return icon_str(Render::Icon::NegativeVolume);
    case Domain::ModelVolumeType::PARAMETER_MODIFIER:
        return icon_str(Render::Icon::ModifierVolume);
    case Domain::ModelVolumeType::SUPPORT_BLOCKER:
        return icon_str(Render::Icon::SupportBlocker);
    case Domain::ModelVolumeType::SUPPORT_ENFORCER:
        return icon_str(Render::Icon::SupportModifier);
    default:
        return "";
    }
}

static std::string volume_icon_tooltip(const Domain::ModelVolume* volume)
{
    switch (volume->type()) {
    case Domain::ModelVolumeType::MODEL_PART:
        return "Solid Part Volume ";
    case Domain::ModelVolumeType::NEGATIVE_VOLUME:
        return "Negative Volume ";
    case Domain::ModelVolumeType::PARAMETER_MODIFIER:
        return "Modifier Volume  ";
    case Domain::ModelVolumeType::SUPPORT_BLOCKER:
        return "Support Blocker ";
    case Domain::ModelVolumeType::SUPPORT_ENFORCER:
        return "Support Modifier";
    default:
        return "";
    }
}

static std::string get_cc_name(const Slic3r::DynamicPrintConfig& print_config)
{
    return icon_str(Render::Icon::ConfigContainer) + print_config.opt_string("printer_model");

    return icon_str(
               Slic3r::Preset::printer_technology(print_config) == ptSLA ?
                   Render::Icon::PrinterSlaIconMarker :
                   Render::Icon::PrinterIconMarker
           )
        + print_config.opt_string("printer_model");
}

static bool bed_has_object(
    const Domain::ModelInstanceList& bed_model_instances,
    const Domain::ModelObject* object
)
{
    for (const Domain::ModelInstance* instance : bed_model_instances)
        if (instance->get_object() == object)
            return true;

    return false;
}

static std::set<size_t> get_object_instance_ids_on_bed(
    const Domain::ModelInstanceList& bed_model_instances,
    const Domain::ModelObject* object
)
{
    std::set<size_t> object_instances_on_bed;

    for (Domain::ModelInstance* instance : bed_model_instances)
        if (instance->get_object() == object)
            object_instances_on_bed.emplace(instance->id().id);

    return object_instances_on_bed;
}

static bool is_whole_object_selected(
    const Domain::ModelObject* object,
    const Slic3r::Biz::Scene::ObjectSelection& selection
)
{
    if (selection.mode == Biz::Scene::SelectionMode::Instance) {
        size_t cnt       = 0;
        size_t object_id = object->id().id;
        for (const Domain::ElementRef& el : selection.elements)
            if (el.object_id == object_id)
                cnt++;
        return cnt == object->instances.size();
    }
    return false;
}

static bool is_volume_selected(
    const Domain::ElementRef& sel_element,
    const Slic3r::Biz::Scene::ObjectSelection& selection
)
{
    if (selection.mode == Biz::Scene::SelectionMode::Volume) {
        for (const Domain::ElementRef& el : selection.elements)
            if (el.object_id == sel_element.object_id && el.volume_id == sel_element.volume_id)
                return true;
    }

    return false;
}

static size_t visible_volumes_count(const Domain::ModelObject* object)
{
    if (object->is_cut()) {
        size_t count{0};
        for (const auto* volume : object->volumes)
            if (!volume->is_cut_connector())
                count++;

        return count;
    }

    return object->volumes.size();
}

static std::set<Render::Icon> get_infos(const Domain::ModelObject* object, bool is_sla_config)
{
    std::set<Render::Icon> infos;
    if (!is_sla_config) {
        for (const Domain::ModelVolume* mv : object->volumes) {
            if (!mv->supported_facets.empty())
                infos.insert(Render::Icon::CustomSupports);
            if (!mv->seam_facets.empty())
                infos.insert(Render::Icon::CustomSeam);
            if (!mv->fuzzy_skin_facets.empty())
                infos.insert(Render::Icon::FuzzySkin);
            if (!mv->mm_segmentation_facets.empty())
                infos.insert(Render::Icon::MmSegmentation);
        }
        if (!object->layer_config_ranges.empty())
            infos.insert(Render::Icon::HRModifier);
        // if (wxGetApp().plater()->canvas3D()->is_object_sinking(obj_idx))
        // infos.insert(ImGui::Sinking);
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
    ImGui::TableSetColumnIndex(ciCount - 1);
    ImVec2 row_end = ImGui::GetCursorScreenPos();
    row_begin.y    = row_end.y; // update row_begin.y
    // get RightBottom for the row
    row_end += ImGui::CalcTextSize(icon_str(Render::Icon::PrintIconMarker).c_str());

    return ImGui::IsMouseHoveringRect(row_begin, row_end, false);
}

// object is simple: has just one instance, one volume and no aditional information
static bool has_overrides(const Domain::ModelObject* object, bool is_sla_config)
{
    bool has_config_overrides = (is_sla_config ? !object->object_settings_sla.overrides.empty() :
                                                 !object->object_settings.overrides.empty())
        || !object->layer_config_ranges.empty();
    if (!has_config_overrides) {
        for (auto volume : object->volumes)
            if (!volume->volume_settings.overrides.empty()) {
                has_config_overrides = true;
                break;
            }
    }
    return has_config_overrides || !get_infos(object, is_sla_config).empty();
}

// object is simple: has just one instance, one volume and no aditional information
static bool is_simple(const Domain::ModelObject* object, bool is_sla_config)
{
    return object->instances.size() == 1
        && object->volumes.size() == 1
        && !has_overrides(object, is_sla_config);
}

static bool is_imgui_item_just_selected()
{
    // handle selection only on MouseRelease or PressEnter
    return ImGui::IsItemHovered(ImGuiHoveredFlags_None)
        && (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter));
}

ObjectList::ObjectList(Biz::ProjectInteractor* project_interactor, ObjectList::Mode mode) : Item()
{
    init(project_interactor, mode);
}

void ObjectList::render(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    auto& ctx          = selected_project_context();
    m_scene_interactor = &m_project_interactor->scene_interactor();
    ctx.model          = &m_project_interactor->selected_project().model();
    ASSERT(ctx.model && m_scene_interactor);

    render_item_begin(pos, size);
    ImGui::SetCursorScreenPos(to_im(pos));

    invalidate_bed_selection();
    update_selection_from_scene();

    selected_project_context().is_dragging = false;

    m_inner_padding = Vec2f(GImGui->FontSize, 1.25f * GImGui->FontSize);

    if (render_list(size)) {
        // update selection on the scene
        propagate_selection();
    }
    process_dragging_start();

    render_item_end(pos, size);
}

// hendle selection from the tree nodes
bool ObjectList::handle_selection(const Domain::ElementRef& id)
{
    auto& ctx = selected_project_context();
    // handle selection only when we are NOT dragging and on MouseRelease or PressEnter
    if (!ctx.is_dragging && is_imgui_item_just_selected()) {
        if (ImGui::GetIO().KeyCtrl) {
            if (ctx.selected_items.count(id))
                ctx.selected_items.erase(id); // Toggle deselect
            else
                ctx.selected_items.insert(id); // Multi-select
        } else {
            ctx.selected_items.clear();
            ctx.selected_items.insert(id); // Single-select
        }
        return true;
    }
    return false;
}

void ObjectList::handle_dragging(const Domain::ElementRef& id)
{
    auto& ctx = selected_project_context();
    if (ctx.scene_map)
        return;

    // Detect dragging on any selected node
    if (ctx.selected_items.count(id)
        && ImGui::IsItemActive()
        && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        ctx.is_dragging = true;
}

void ObjectList::force_select_whole_object(const Domain::ModelObject* object)
{
    auto& ctx        = selected_project_context();
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{object_id};
    if (ctx.selected_items.count(sel_element)) {
        // remove object element
        ctx.selected_items.erase(sel_element);
        // and push all instances instad
        for (const Domain::ModelInstance* instance : object->instances)
            ctx.selected_items.insert(Domain::ElementRef{object_id, instance->id().id});
    }
}

using namespace Imgui;

void ObjectList::init(Biz::ProjectInteractor* project_interactor, Mode mode)
{
    m_project_interactor = project_interactor;
    m_mode               = mode;
    m_project_contexts   = std::make_unique<ProjectContexts>(*project_interactor);

    m_multi_selection_flags = ImGuiMultiSelectFlags_ScopeRect
        | ImGuiMultiSelectFlags_ClearOnEscape
        | ImGuiMultiSelectFlags_BoxSelect1d
        | ImGuiMultiSelectFlags_SelectOnClick;

    m_node_flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_FramePadding
        | ImGuiTreeNodeFlags_SpanAllColumns;

    m_table_flags = ImGuiTableFlags_NoBordersInBody |
        // ImGuiTableFlags_Borders |
        ImGuiTableFlags_NoPadInnerX;
}

void ObjectList::set_bed_instance_icons(const Plater::BedThumbnailTextures& icons)
{
    auto& ctx              = selected_project_context();
    ctx.bed_instance_icons = icons;
}

void ObjectList::process_dragging_start()
{
    const auto& ctx = selected_project_context();
    // Start drag operation when any selected node is being dragged
    if (ctx.is_dragging
        && ImGui::BeginDragDropSource(
            /*ImGuiDragDropFlags_SourceNoHoldToOpenOthers | */ ImGuiDragDropFlags_SourceExtern
        ))
    {
        int size = (int) ctx.selected_items.size();
        assert(size > 0);
        if (ctx.selected_items.begin()->has_volume()) {
            ImGui::Text("Mixing volume(s)");
            ImGui::SetDragDropPayload("MULTI_VOLUMES", &size, sizeof(int));
        } else {
            ImGui::Text("Extracting instances into separate object");
            ImGui::SetDragDropPayload("MULTI_INSTANCES", &size, sizeof(int));
        }
        ImGui::EndDragDropSource();
    }
}

void ObjectList::update_selection_from_scene()
{
    auto& ctx                                    = selected_project_context();
    const Biz::Scene::ObjectSelection& scene_selection = m_scene_interactor->object_selection();
    for (const Domain::ModelObject* object : ctx.model->objects) {
        size_t object_id = object->id().id;

        MultiSelectionStorage& inst_ms = ctx.instances_ms.get_ms<Domain::ModelInstancePtrs>(object_id);
        inst_ms.UserData = (void*) &object->instances;

        MultiSelectionStorage& vol_ms = ctx.volumes_ms.get_ms<Domain::ModelVolumePtrs>(object_id);
        vol_ms.UserData               = (void*) &object->volumes;
    }

    std::set<Domain::ElementRef> selected_items_tmp = std::set<Domain::ElementRef>(
        scene_selection.elements.begin(),
        scene_selection.elements.end()
    );
    if (selected_items_tmp != ctx.selected_items) {
        clear_all_ms();
        if (scene_selection.mode == Biz::Scene::SelectionMode::Volume) {
            for (const auto& el : scene_selection.elements)
                ctx.volumes_ms.at(el.object_id).SetItemSelected(el.volume_id, true);
        } else if (scene_selection.mode == Biz::Scene::SelectionMode::Instance) {
            for (const auto& el : scene_selection.elements) {
                ctx.instances_ms.at(el.object_id).SetItemSelected(el.instance_id, true);
            }
        }
        ctx.selected_items = selected_items_tmp;
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

bool ObjectList::tree_node(
    const char* str_id,
    ImGuiTreeNodeFlags flags,
    const std::string& label,
    bool add_overrides_marker /* = false*/,
    unsigned long long tex_id /* = 0*/,
    ImVec2 icon_size /* = {0.0f, 0.0f}*/
)
{
    DEBUG_ASSERT(tex_id == 0 || (icon_size.x > 0.0f && icon_size.y > 0.0f));
    if (tex_id == 0)
        icon_size = ImVec2(0.0f, 0.0f);

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

    ImGuiContext& g         = *GImGui;
    const ImGuiStyle& style = g.Style;
    ImGuiWindow* window     = ImGui::GetCurrentWindow();
    const ImVec2 padding    = ImVec2(
        style.FramePadding.x,
        ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y)
    );

    const float text_offset_x = g.FontSize
        + icon_size.x
        + padding.x * 3; // Collapsing arrow width + icon width + Spacing
    const float text_offset_y = ImMax(
        padding.y,
        window->DC.CurrLineTextBaseOffset
    ); // Latch before ItemSize changes it

    ImVec2 text_pos(pos_old.x + text_offset_x, pos_old.y + text_offset_y + style.FramePadding.y);

    ImVec2 pos     = ImVec2(text_pos.x - text_offset_x, text_pos.y);
    ImVec2 pos_end = pos + ImVec2(g.FontSize, g.FontSize);

    // render rect over the triangle
    ImDrawList* draw_list = ImGui::GetCurrentWindow()->DrawList;

    ImGuiID active_id = ImGui::GetActiveID();

    // first layer
    draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_WindowBg));
    if (row_color != ImGuiCol_COUNT)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(row_color));

    ImGuiID edit_name_input_id = ImGui::GetID("##edit");

    // second layer
    if (ImGui::IsItemActive() && ImGui::IsItemHovered())
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_HeaderActive));
    else if (ImGui::IsItemHovered() && !m_is_edit_name_input_hovered)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
    else if (is_selected)
        draw_list->AddRectFilled(pos, pos_end, ImGui::GetColorU32(ImGuiCol_Header));

    // render open-close new arrow
    draw_list->AddText(
        pos,
        ImGui::GetColorU32(ImGuiCol_Text),
        icon_str(is_open ? Render::Icon::OpenArrow : Render::Icon::CloseArrow).c_str()
    );

    if (add_overrides_marker)
        draw_list->AddText(
            text_pos,
            ImGui::GetColorU32(ImGuiCol_Text),
            icon_str(Render::Icon::OverridesMarker).c_str()
        );

    if (tex_id != 0) {
        ImVec2 icon_pos = ImVec2(
            pos_end.x + style.ItemInnerSpacing.x,
            pos_old.y + style.ItemInnerSpacing.y
        );
        ImVec2 icon_pos_end = icon_pos + icon_size;
        draw_list->AddImage(tex_id, icon_pos, icon_pos_end);
    }

    // revert cursor position
    ImGui::SetCursorScreenPos(pos_new);
    return is_open;
}

static bool selectable(
    const char* label,
    bool selected              = false,
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns,
    bool add_overrides_marker  = false
)
{
    ImVec2 init_pos(ImGui::GetCursorScreenPos());

    ImGuiStyle style = GImGui->Style;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 2 * style.FramePadding.y));
    bool ret = ImGui::Selectable(label, selected, flags);
    ImGui::PopStyleVar();

    if (add_overrides_marker) {
        ImGui::SameLine(init_pos.x - style.ItemSpacing.x - style.CellPadding.x);
        ImGui::Text(icon_str(Render::Icon::OverridesMarker).c_str());
        ImGui::SameLine();
    }

    return ret;
}

bool ObjectList::render_config_containers()
{
    auto& ctx                 = selected_project_context();
    bool is_changed_selection = false;

    size_t beds_cnt{0};
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
            if (ctx.selected_bed_instance_id != 0) {
                ctx.selected_container_id = cc->id().id;
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
    size_t total_beds_cnt    = 0;
    size_t finished_beds_cnt = 0;
    for (auto& cc : m_scene_interactor->selected_project_config_containers()) {
        for (auto& bed_inst : cc->bed_instances()) {
            if (bed_inst->model_instances.empty())
                continue;
            total_beds_cnt++;
            const std::optional<Biz::Slicing::Status> status{
                m_project_interactor->status_cache()
                    .get_status({m_project_interactor->selected_project_id(), bed_inst->id().id})
            };
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
        render_group_name(icon_str(Render::Icon::AllBeds) + " All");

        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 0.25f * m_inner_padding.y()));

        BoldFontGuard bfg(m_imgui_render);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::GetColorU32(BLUE_BUTTON_COLOR));
        if (total_beds_cnt == finished_beds_cnt)
            ImGui::ProgressBar(1.0, progress_bar_sz, "SLICED");
        else
            ImGui::ProgressBar(
                float(finished_beds_cnt) / float(total_beds_cnt),
                progress_bar_sz,
                Slic3r::format("%1%/%2% SLICED", finished_beds_cnt, total_beds_cnt).c_str()
            );
        ImGui::PopStyleColor();

        ImGui::EndTable();
    }
}

bool ObjectList::render_out_of_beds()
{
    const auto& ctx = selected_project_context();
    if (m_mode == Mode::Preview
        || m_scene_interactor->selected_project_unplaced_model_instances().empty())
        return false;

    bool is_changed_selection = false;

    render_group_name(L("Out of bed"));

    BedsTable table;
    if (table.begin(size_t(-1), m_table_flags)) {
        IndentGuard ig(m_inner_padding.x());
        for (const Domain::ModelObject* object : ctx.model->objects) {
            if (bed_has_object(m_scene_interactor->selected_project_unplaced_model_instances(), object))
                is_changed_selection |= render_object_node(object);
        }
    }

    return is_changed_selection;
}

void ObjectList::render_drop_target_area()
{
    auto& ctx = selected_project_context();
    if (m_mode != Mode::Plater)
        return;

    // Make the entire window a valid drop target
    const float drop_area_height = 50.f;
    ImGui::InvisibleButton(
        "InstancesDropZone",
        ImVec2(ImGui::GetContentRegionAvail().x, drop_area_height)
    ); // Creates an invisible instances drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MULTI_INSTANCES")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            ask_extract_selected_instances();
            ctx.selected_items.clear(); // Clear selection after drop
        }
        ImGui::EndDragDropTarget();
    }
}

bool ObjectList::render_bed_node(const Domain::BedInstance* bed, size_t config_container_id)
{
    auto& ctx          = selected_project_context();
    size_t bed_id      = bed->id().id;
    bool is_sla_config = m_project_interactor->selected_project()
                             .find_config_container(config_container_id)
                             ->print_technology()
        == Domain::PrinterTechnology::SLA;

    const std::string name    = "Bed " + bed->label();
    const std::string name_id = "##bed_id" + std::to_string(bed_id);

    const ImGuiStyle& style = ImGui::GetStyle();

    const ImVec2 icon_size = ImVec2(40.f, 40.f);
    const ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
    const ImVec2 padding   = style.ItemInnerSpacing;

    const bool is_active{
        m_scene_interactor->bed_selection().is_selected(Domain::BedRef{config_container_id, bed_id})
    };
    RowBackground bg(is_active);
    new_row(icon_size.y + 2.f * padding.y);

    ImTextureID tex_id = 0;
    auto it            = std::find_if(
        ctx.bed_instance_icons.begin(),
        ctx.bed_instance_icons.end(),
        [&](const Plater::BedThumbnailTexture& tt) {
            return tt.bed_ref.config_container_id == config_container_id
                && tt.bed_ref.instance_id == bed_id;
        }
    );
    if (it != ctx.bed_instance_icons.end())
        tex_id = (ImTextureID) (intptr_t) it->thumbnail.get();

    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding,
        ImVec2(0.5f * icon_size.x + padding.x, 0.5f * (icon_size.y - text_size.y) + padding.y)
    );
    bool is_open = tree_node(
        name_id.c_str(),
        m_node_flags | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap,
        name,
        false,
        tex_id,
        icon_size
    );
    ImGui::PopStyleVar();

    bool is_changed_selection = false;
    // check bed selection
    if (!ctx.is_dragging && is_imgui_item_just_selected()) {
        ctx.selected_bed_instance_id = bed->id().id;
        is_changed_selection         = true;
    }

    render_slicing_state_marker(bed_id);
    if (!is_sla_config)
        render_extruder_marker(0, {"#E74840"});

    if (is_open) {
        bg.set_next();
        for (const Domain::ModelObject* object : ctx.model->objects) {
            if (bed_has_object(bed->model_instances, object))
                is_changed_selection |= render_object_node(object, bed, is_sla_config);
        }
        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_object_node(
    const Domain::ModelObject* object,
    const Domain::BedInstance* bed /*= nullptr*/,
    bool is_sla_config /*= false*/
)
{
    auto& ctx        = selected_project_context();
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{object_id};

    bool is_selected = is_whole_object_selected(object, m_scene_interactor->object_selection());
    if (ctx.edited_node_id == object_id && !is_selected)
        ctx.edited_node_id = 0; // Exit edit mode

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
    if (ctx.edited_node_id == object_id && is_selected) {
        isOpen = tree_node(
            name_id.c_str(),
            flags | ImGuiTreeNodeFlags_AllowOverlap,
            icon_str(Render::Icon::ObjectIcon),
            has_overrides(object, is_sla_config)
        );
        render_edited(name.c_str(), {object_id});
    } else
        isOpen = tree_node(
            name_id.c_str(),
            flags | ImGuiTreeNodeFlags_AllowOverlap,
            (icon_str(Render::Icon::ObjectIcon) + name),
            has_overrides(object, is_sla_config)
        );

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection) {
        force_select_whole_object(object);
        ctx.volumes_ms.clear_all();
        if (ImGui::GetIO().KeyCtrl)
            ctx.instances_ms.at(object_id).Clear();
        else
            ctx.instances_ms.clear_all();
        ctx.edited_node_id = object_id;
    }

    handle_dragging(sel_element);

    render_printable_icon(sel_element, object->printable);
    if (!is_sla_config)
        render_extruder_marker(1, {"#240E74", "#E74840", "#FAD73B"});

    if (isOpen) {
        bg.set_next();
        is_changed_selection |= render_connectors_node(object, bed ? bed->id().id : 0);
        is_changed_selection |= render_volumes(object, bed ? bed->id().id : 0, is_sla_config);
        is_changed_selection |= render_instances_node(object, bed);
        if (ctx.show_details)
            render_infos_node(object, is_sla_config);

        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_connectors_node(const Domain::ModelObject* object, size_t bed_id)
{
    if (!object->is_cut() || object->volumes.size() == 1)
        return false;

    auto& ctx          = selected_project_context();
    size_t object_id   = object->id().id;
    size_t instance_id = object->instances[0]->id().id;
    const std::string name_id = "##connectors_id" + std::to_string(bed_id) + std::to_string(object_id);

    new_row();
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    if (tree_node(
            name_id.c_str(),
            m_node_flags | ImGuiTreeNodeFlags_Leaf,
            icon_str(Render::Icon::CutConnectors) + "Connectors"
        ))
        ImGui::TreePop();
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    if (handle_selection({object_id, instance_id})) {
        clear_all_ms();

        std::set<Domain::ElementRef> selected_items_tmp;
        for (const Domain::ModelVolume* volume : object->volumes) {
            if (volume->is_cut_connector())
                selected_items_tmp.insert(Domain::ElementRef{object_id, instance_id, volume->id().id});
        }
        if (ctx.selected_items != selected_items_tmp) {
            ctx.selected_items = selected_items_tmp;
            return true;
        }
    }

    return false;
}

bool ObjectList::render_volumes(const Domain::ModelObject* object, size_t bed_id, bool is_sla_config)
{
    auto& ctx = selected_project_context();
    if (visible_volumes_count(object) < 2)
        return false;

    size_t object_id   = object->id().id;
    size_t instance_id = object->instances[0]->id().id;

    const std::string name_id = "##volumes_id" + std::to_string(bed_id) + std::to_string(object_id);

    new_row();
    bool is_open = tree_node(name_id.c_str(), m_node_flags, "Volumes");

    const Domain::ModelVolumePtrs& volumes = object->volumes;
    MultiSelectionStorage& ms              = ctx.volumes_ms.at(object_id);

    if (is_open) {
        ImGui::PushID(&ms); // Ensure unique ID
        ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(
            m_multi_selection_flags,
            ms.Size,
            visible_volumes_count(object)
        );
        ms.ApplyRequests(ms_io);

        for (size_t vol_id = 0; vol_id < volumes.size(); vol_id++) {
            const Domain::ModelVolume* volume = object->volumes[vol_id];
            if (volume->is_cut_connector())
                continue;
            size_t volume_id = volume->id().id;
            ImGui::SetNextItemSelectionUserData(vol_id);
            render_volume_node(
                volume,
                {object_id, instance_id, volume_id},
                ms.Contains((ImGuiID) volume_id),
                is_sla_config
            );
        }

        ms_io = ImGui::EndMultiSelect();
        ms.ApplyRequests(ms_io);
        ImGui::PopID();

        ImGui::TreePop();
    }
    // update selection
    if (ms.is_changed) {
        ctx.instances_ms.clear_all();
        ctx.volumes_ms.clear_except(object_id);

        std::set<Domain::ElementRef> selected_items_tmp;
        for (const Domain::ModelVolume* volume : volumes) {
            size_t volume_id = volume->id().id;
            if (ms.Contains((ImGuiID) volume_id))
                selected_items_tmp.insert(Domain::ElementRef{object_id, instance_id, volume_id});
        }

        if (ctx.selected_items != selected_items_tmp) {
            ctx.selected_items = selected_items_tmp;
            return true;
        }
    }

    return false;
}

// render edited item as an input text and propagate new name to scene_interactor
void ObjectList::render_volume_node(
    const Domain::ModelVolume* volume,
    const Domain::ElementRef& sel_element,
    bool is_selected,
    bool is_sla_config
)
{
    auto& ctx = selected_project_context();
    std::string
        volume_name = (volume->name.empty() ? "Volume " + std::to_string(volume->id().id) : volume->name);
    size_t volume_id = volume->id().id;

    if (ctx.edited_node_id == volume_id && !is_selected)
        ctx.edited_node_id = 0; // Exit edit mode

    NewRowWithSelectable row;
    ImVec2 pos                  = ImGui::GetCursorScreenPos();
    bool has_config_overrides   = !volume->volume_settings.overrides.empty();
    bool has_extruder_overrides = !is_sla_config
        && has_config_overrides
        && volume->volume_settings.overrides.get("extruder").has_value();

    if (ctx.edited_node_id == volume_id) {
        if (selectable(
                icon_str(volume).c_str(),
                is_selected,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                has_config_overrides
            ))
        {
            ctx.edited_node_id = 0; // Discard edit mode on selection
        }
        render_edited(volume_name.c_str(), sel_element);

        // During rendering of the InputText different values of cursore position are changed.
        // As a result positions of other items in the row are missed up.
        // But invalidating of the RowCellPaddingY fixes this issue
        GImGui->CurrentTable->RowCellPaddingY = 0.f;
    } else {
        // Display as a selectable label
        if (selectable(
                (icon_str(volume) + volume_name).c_str(),
                is_selected,
                ImGuiSelectableFlags_SpanAllColumns,
                has_config_overrides
            ))
            ctx.edited_node_id = volume_id; // Start edit mode on selection
    }

    const ImVec2 size = ImGui::CalcTextSize(icon_str(volume).c_str());
    if (ImGui::IsMouseHoveringRect(pos, pos + size))
        Imgui::tooltip(volume_icon_tooltip(volume));

    if (has_extruder_overrides)
        render_extruder_marker(2, {"#40E740"});
}

bool ObjectList::render_instances_node(const Domain::ModelObject* object, const Domain::BedInstance* bed /*= nullptr*/)
{
    if (object->instances.size() == 1)
        return false;
    std::set<size_t> instances_on_bed = get_object_instance_ids_on_bed(
        bed ? bed->model_instances : m_scene_interactor->selected_project_unplaced_model_instances(),
        object
    );
    if (instances_on_bed.empty())
        return false;

    auto& ctx        = selected_project_context();
    size_t object_id = object->id().id;
    Domain::ElementRef sel_element{object_id};

    MultiSelectionStorage& ms = ctx.instances_ms.at(object_id);

    const std::string name_id = "Instances##obj_id" + std::to_string(object_id);

    new_row();
    bool isOpen = tree_node(
        name_id.c_str(),
        m_node_flags | ImGuiTreeNodeFlags_DefaultOpen,
        icon_str(Render::Icon::InstancesIcon) + "Instances"
    );

    bool is_changed_selection = handle_selection(sel_element);
    if (is_changed_selection) {
        force_select_whole_object(object);
        ctx.volumes_ms.clear_all();
        if (ImGui::GetIO().KeyCtrl)
            ms.Clear();
        else
            ctx.instances_ms.clear_all();
    }

    handle_dragging(sel_element);

    if (isOpen) {
        is_changed_selection |= render_instances(object, instances_on_bed);
        ImGui::TreePop();
    }

    return is_changed_selection;
}

bool ObjectList::render_instances(const Domain::ModelObject* object, const std::set<size_t>& instances_on_bed)
{
    auto& ctx                                  = selected_project_context();
    const Domain::ModelInstancePtrs& instances = object->instances;
    size_t object_id                           = object->id().id;

    MultiSelectionStorage& ms = ctx.instances_ms.at(object_id);
    ImGui::PushID(&ms);
    ImGuiMultiSelectIO* ms_inst_io = ImGui::BeginMultiSelect(
        m_multi_selection_flags,
        ms.Size,
        instances_on_bed.size()
    );
    ms.ApplyRequests(ms_inst_io);

    for (size_t inst_id = 0; inst_id < instances.size(); inst_id++) {
        size_t instance_id = instances[inst_id]->id().id;
        if (instances_on_bed.count(instance_id))
            render_instance_node(object, inst_id, ms.Contains((ImGuiID) instance_id));
    }

    ms_inst_io = ImGui::EndMultiSelect();
    ms.ApplyRequests(ms_inst_io);
    ImGui::PopID();

    // update selection
    bool is_changed_selection = false;
    if (ms.is_changed) {
        ctx.volumes_ms.clear_all();

        if (!ImGui::GetIO().KeyCtrl) {
            ctx.instances_ms.clear_except(object_id);
            ctx.selected_items.clear();
        }

        for (const Domain::ModelInstance* instance : instances) {
            size_t instance_id = instance->id().id;
            if (instances_on_bed.count(instance_id)) {
                Domain::ElementRef sel_element{object_id, instance->id().id};

                if (ms.Contains((ImGuiID) instance_id) && !ctx.selected_items.count(sel_element)) {
                    is_changed_selection = true;
                    ctx.selected_items.insert(sel_element);
                } else if (!ms.Contains((ImGuiID) instance_id)
                           && ctx.selected_items.count(sel_element))
                {
                    is_changed_selection = true;
                    ctx.selected_items.erase(sel_element);
                }
            }
        }
    }

    return is_changed_selection;
}

void ObjectList::render_instance_node(const Domain::ModelObject* object, size_t inst_id, bool is_selected)
{
    const Domain::ModelInstance* instance = object->instances[inst_id];
    size_t id                             = instance->id().id;
    Domain::ElementRef sel_element{object->id().id, id, 0};

    const std::string name = icon_str(Render::Icon::ObjectIcon) + "Instance " + std::to_string(id);

    NewRowWithSelectable row;
    ImGui::SetNextItemSelectionUserData(inst_id);
    selectable(
        name.c_str(),
        is_selected,
        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap
    );

    render_printable_icon(sel_element, instance->printable);

    handle_dragging(sel_element);
}

void ObjectList::render_infos_node(const Domain::ModelObject* object, bool is_sla_config)
{
    const std::set<Render::Icon> infos = get_infos(object, is_sla_config);
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
        propagate_name_editing(sel_element, buffer); // Save edited name
        auto& ctx          = selected_project_context();
        ctx.edited_node_id = 0; // Exit edit mode
    }
    // save hovered state for InputText => will be used in tree_node rendering
    m_is_edit_name_input_hovered = ImGui::IsItemHovered();

    ImGui::PopStyleVar(2);
}

void ObjectList::render_printable_icon(const Domain::ElementRef& sel_id, bool is_printable)
{
    Render::Icon icon = Render::Icon::None;
    if (hovered_current_row()) {
        icon = is_printable ? Render::Icon::EyeOpen : Render::Icon::EyeClosed;
    } else if (!is_printable) {
        icon = Render::Icon::EyeClosed;
    } else
        return;

    ImGui::TableSetColumnIndex(ciPrintable);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_Button));
    ImGui::PushID(
        Slic3r::format("##print_%1%_%2%_%3%", sel_id.object_id, sel_id.instance_id, sel_id.volume_id)
            .c_str()
    ); // Ensure unique ID
    if (button_aligned(1.f, icon_str(icon), ImVec2(), ImGuiButtonFlags_AlignTextBaseLine))
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
        colors.push_back({clr.r(), clr.g(), clr.b(), 1.f});
    }

    colored_circle_marker_aligned(0.5f, str_colors.size() == 1 ? std::to_string(extruder_id) : "", colors);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void ObjectList::render_slicing_state_marker(size_t bed_instance_id)
{
    const std::optional<Biz::Slicing::Status> status{m_project_interactor->status_cache().get_status(
        {m_project_interactor->selected_project_id(), bed_instance_id}
    )};
    if (!status || status == Biz::Slicing::Status::Empty)
        return;

    const float align_x{1.f};

    ImGui::TableSetColumnIndex(ciPrintable);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 1.f));
    if (status == Biz::Slicing::Status::Finished) {
        BoldFontGuard bfg(m_imgui_render);
        text_with_bg_aligned(align_x, L("SLICED"), BLUE_BUTTON_COLOR);
    } else if (status != Biz::Slicing::Status::Modified) {
        text_with_bg_aligned(align_x, L("SLICING"), {0.32f, 0.48f, 0.84f, 0.65f});
    } else if (m_mode == Mode::Preview) {
        BoldFontGuard bfg(m_imgui_render);
        ImGui::PushStyleColor(ImGuiCol_Button, ORANGE_BUTTON_COLOR);
        if (button_aligned(align_x, L("SLICE"), ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine))
            m_project_interactor->slicing_interactor().slice_bed(bed_instance_id);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
}

static std::map<Render::Icon, std::string> info_descriptions = {
    {Render::Icon::CustomSupports, "CustomSupports"},
    {Render::Icon::CustomSeam, "Seam"},
    {Render::Icon::MmSegmentation, "MM Painting"},
    {Render::Icon::Sinking, "Sinking"},
    {Render::Icon::FuzzySkin, "Fuzzy Skin"},
    {Render::Icon::HRModifier, "Height range Modifier"},
};

void ObjectList::render_infos_selectable(
    const std::set<Render::Icon>& infos,
    const Domain::ModelObject* object,
    bool force_render
)
{
    for (Render::Icon info : infos) {
        NewRowWithSelectable row;
        std::string line = icon_str(info) + info_descriptions[info];
        if (selectable(line.c_str())) {
            if (info == Render::Icon::Sinking || info == Render::Icon::HRModifier) {
                force_select_whole_object(object);
                clear_all_ms();
            } else
                show_gizmo({object->id().id}, info);
        }
    }
}

void ObjectList::clear_all_ms()
{
    auto& ctx = selected_project_context();
    ctx.instances_ms.clear_all();
    ctx.volumes_ms.clear_all();
}

void ObjectList::invalidate_bed_selection()
{
    auto& ctx                    = selected_project_context();
    ctx.selected_container_id    = 0;
    ctx.selected_bed_instance_id = 0;
}

void ObjectList::propagate_selection()
{
    const auto& ctx = selected_project_context();
    if (ctx.selected_bed_instance_id != 0) {
        m_scene_interactor->select_one_bed_instance(
            {ctx.selected_container_id, ctx.selected_bed_instance_id}
        );
        return;
    }

    Biz::Scene::ObjectSelection sels;
    sels.elements = std::vector<Domain::ElementRef>(
        ctx.selected_items.begin(),
        ctx.selected_items.end()
    );
    sels.normalize();
    m_scene_interactor->set_object_selection(sels);
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

void ObjectList::show_gizmo(const Domain::ElementRef& sel_element, Render::Icon gizmo_id)
{
    // ToDo
}

ObjectList::ProjectContext& ObjectList::selected_project_context()
{
    return m_project_contexts->selected();
}

const ObjectList::ProjectContext& ObjectList::selected_project_context() const
{
    return m_project_contexts->selected();
}

} // namespace Slic3r::App
