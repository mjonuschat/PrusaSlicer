#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

namespace Slic3r::App::Preview {

static bool add_centered_icon_button(
    wchar_t icon, const std::string& id, const std::string& tooltip = std::string()
)
{
    ImGui::TableNextColumn();

    float h = 1.25f * ImGui::GetTextLineHeight();
    ImVec2 btn_sz(h, h);
    float offsetX = (ImGui::GetColumnWidth() - btn_sz.x) * 0.5 - GImGui->Style.FramePadding.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

    bool clicked = Imgui::icon_button(icon, btn_sz, id);
    ImGui::SetItemTooltip("%s", tooltip.c_str());
    return clicked;
}

SidebarPreviewActionButtons::SidebarPreviewActionButtons(Item* parent)
    : SidebarActionButtons("sidebar_preview_action_buttons", Render::ModuleType::Preview)
{
    set_min_size({220, 80});
}

void SidebarPreviewActionButtons::render_body(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    render_export_buttons();

    // ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.f);
    ImGui::PushFont(m_imgui_render->font(Render::ImguiFontType::Bold));

    render_navigation_button();
    ImGui::SameLine();
    render_slice_button(size);

    ImGui::PopFont();
}

bool SidebarPreviewActionButtons::export_allowed() const
{
    const Biz::Slicing::SlicingId id = m_project_interactor->selected_bed_slicing_id();
    const std::optional<Biz::Slicing::Status> status{
        m_project_interactor->status_cache().get_status(id)
    };

    return status && status == Biz::Slicing::Status::Finished;
}

void SidebarPreviewActionButtons::render_export_buttons()
{
    const bool is_export_allowed = export_allowed();
    if (!is_export_allowed)
        ImGui::BeginDisabled();

    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_NoPadInnerX;
    if (ImGui::BeginTable("##ObjectListTable", 4, table_flags)) {
        if (add_centered_icon_button(ImGui::SavePrint, "SavePrint", "Export")) {
            m_project_interactor->do_export(active_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintToFlash, "SavePrintToFlash", "Export to flash")) {
            m_project_interactor->do_export(active_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintToLocal, "SavePrintToLocal", "Export to local")) {
            m_project_interactor->do_export(active_bed_slicing_id(), {});
        }
        if (add_centered_icon_button(ImGui::SavePrintAddBookmark, "SavePrintAddBookmark", "Upload")) {
            m_project_interactor->do_upload(active_bed_slicing_id());
        }

        ImGui::EndTable();
    }

    if (!is_export_allowed)
        ImGui::EndDisabled();
}

const static float navig_btn_width = 40.f;
const static float btns_height = 45.f;

void SidebarPreviewActionButtons::render_navigation_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.32f, 0.48f, 0.84f, 1.0f));
    if (ImGui::Button(m_navigator_name.c_str(), ImVec2(navig_btn_width, btns_height)))
        navigate_to_other();
    ImGui::SetItemTooltip("%s", m_navigator_tooltip.c_str());
    ImGui::PopStyleColor();
}

void SidebarPreviewActionButtons::render_slice_button(Domain::Vec2f size)
{
    float slice_btn_width = size.x() - GImGui->Style.ItemSpacing.x - navig_btn_width;

    const bool is_slice_allowed = slice_allowed();
    if (!is_slice_allowed)
        ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.99f, 0.41f, 0.2f, 1.0f));
    if (ImGui::Button("Slice", ImVec2(slice_btn_width, btns_height))) {
        m_project_interactor->slicing_interactor().slice_bed(active_bed_slicing_id().bed_instance_id
        );
        navigate_to_other();
    }
    ImGui::PopStyleColor();

    if (!is_slice_allowed)
        ImGui::EndDisabled();
}

} // namespace Slic3r::App::Preview
