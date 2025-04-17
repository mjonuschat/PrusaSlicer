#include "Slic3r/App/SidebarActionButtons.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using RMType = Render::ModuleType;

void SidebarActionButtons::on_init(Biz::ProjectInteractor* project_interactor, Render::ImguiRender* imgui_render, Render::ModuleType type)
{
    ASSERT(type != RMType::Undef);

    m_project_interactor = project_interactor;
    m_imgui_render       = imgui_render;
    m_type               = type;

    m_navigator_name    = m_type == RMType::Plater ? ">" : "<";
    m_navigator_tooltip = m_type == RMType::Plater ? "Show Preview" : "Back to Plater";
    m_navigate_to_type  = m_type == RMType::Plater ? RMType::Preview : RMType::Plater;
}

bool SidebarActionButtons::slice_allowed() const
{
    const Biz::Slicing::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status {
        m_project_interactor->status_cache().get_status(id) };

    return status && status == Biz::Slicing::Status::Modified;
}

bool SidebarActionButtons::export_allowed() const
{
    const Biz::Slicing::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status {
        m_project_interactor->status_cache().get_status(id) };

    return status && status == Biz::Slicing::Status::Finished;
}

static bool add_centered_icon_button(wchar_t icon, const std::string& id, const std::string& tooltip = std::string())
{
    ImGui::TableNextColumn();

    float h = 1.25f * ImGui::GetTextLineHeight();
    ImVec2 btn_sz(h, h);
    float offsetX = (ImGui::GetColumnWidth() - btn_sz.x) * 0.5 - GImGui->Style.FramePadding.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

    bool clicked = Imgui::icon_button(icon, btn_sz, id);
    ImGui::SetItemTooltip(tooltip.c_str());
    return clicked;
}

void SidebarActionButtons::navigate_to_other()
{
    invoke_listeners<IRenderModuleChangedListener>([this](auto* listener) {
        listener->on_render_module_changed(m_navigate_to_type); 
    });
}

void SidebarActionButtons::render_export_buttons()
{
    const bool is_export_allowed = export_allowed();
    if (!is_export_allowed)
        ImGui::BeginDisabled();

    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadInnerX;
    if (ImGui::BeginTable("##ObjectListTable", 4, table_flags)) {
        if (add_centered_icon_button(ImGui::SavePrint, "SavePrint", "Export")) {
            m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintToFlash, "SavePrintToFlash", "Export to flash")) {
            m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintToLocal, "SavePrintToLocal", "Export to local")) {
            m_project_interactor->do_export(m_project_interactor->selected_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintAddBookmark, "SavePrintAddBookmark", "Upload")) {
            m_project_interactor->do_upload(m_project_interactor->selected_bed_slicing_id());
        }

        ImGui::EndTable();
    }

    if (!is_export_allowed)
        ImGui::EndDisabled();
}

const static float navig_btn_width = 40.f;
const static float btns_height = 45.f;

void SidebarActionButtons::render_navigation_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.32f, 0.48f, 0.84f, 1.0f));
    if (ImGui::Button(m_navigator_name.c_str(), ImVec2(navig_btn_width, btns_height)))
        navigate_to_other();
    ImGui::SetItemTooltip(m_navigator_tooltip.c_str());
    ImGui::PopStyleColor();
}

void SidebarActionButtons::render_slice_button(Domain::Vec2f size)
{
    float slice_btn_width = size.x() - GImGui->Style.ItemSpacing.x - navig_btn_width;

    const bool is_slice_allowed = slice_allowed();
    if (!is_slice_allowed)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    if (ImGui::Button("Slice", ImVec2(slice_btn_width, btns_height))) {
        m_project_interactor->slicing_interactor().slice_bed(m_project_interactor->selected_bed_slicing_id().bed_instance_id);
        navigate_to_other();
    }
    ImGui::PopStyleColor();

    if (!is_slice_allowed)
        ImGui::EndDisabled();
}

void SidebarActionButtons::render(Domain::Vec2f pos, Domain::Vec2f size)
{
    render_export_buttons();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.f);
    ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));

    render_navigation_button();
    ImGui::SameLine();
    render_slice_button(size);

    ImGui::PopFont();
}

}// Slic3r::App namespace
