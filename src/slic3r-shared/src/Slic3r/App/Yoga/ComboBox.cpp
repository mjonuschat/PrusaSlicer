///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ComboBox.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

/**
 * @note copied from imgui internals, we need our custom styling
 */
static void YGRenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    // clang-format off
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.35f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        b = ImVec2(+0.000f,+0.750f) * r;
        a = ImVec2(-0.750f,-0.00f) * r;
        c = ImVec2(+0.750f,-0.00f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        b = ImVec2(+0.750f,+0.000f) * r;
        c = ImVec2(-0.000f,+0.750f) * r;
        a = ImVec2(-0.000f,-0.750f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }

    if ((col & IM_COL32_A_MASK) == 0)
        return;

    draw_list->PathLineTo(center + a);
    draw_list->PathLineTo(center + b);
    draw_list->PathLineTo(center + c);
    draw_list->PathStroke(col, false, 2.);
    // clang-format on
}

/**
 * @note copied from imgui internals, we need our custom styling
 */
static bool YGBeginCombo(
    const char* label,
    const char* preview_value,
    ImVec2 size_arg,
    ImGuiComboFlags flags,
    bool editable,
    char* buffer,
    int buf_size,
    bool& edited,
    Validator* validator,
    ComboBox::Callbacks& callbacks
)
{
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    // clang-format off
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | (ImGuiComboFlags)ImGuiComboFlags_CustomPreview)) == 0);


    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : ImGui::GetFrameHeight();
    const float preview_width = ((flags & ImGuiComboFlags_WidthFitPreview) && (preview_value != NULL)) ? ImGui::CalcTextSize(preview_value, NULL, true).x : 0.0f;
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : ((flags & ImGuiComboFlags_WidthFitPreview) ? (arrow_size + preview_width + style.FramePadding.x * 2.0f) : ImGui::CalcItemWidth());

    const ImVec2 frame_size = ImGui::CalcItemSize(size_arg, w, style.FramePadding.y * 2.0f);

    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect total_bb(bb.Min, bb.Max);
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &bb))
        return false;

    // Open on click
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    // Render shape
    const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    ImGui::RenderNavCursor(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(bb.Min, bb.Max, frame_col, style.FrameRounding, ImDrawFlags_RoundCornersAll);
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x) {
            YGRenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.x, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
        }
    }
    ImGui::RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Custom preview
    if (flags & ImGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    if (editable) {
        ImGui::SetItemAllowOverlap();
        ImGui::SetCursorScreenPos(cursor_pos);
        edited |= ImGui::InputTextEx("##foo", "", buffer, buf_size, ImVec2(value_x2 - bb.Min.x, bb.Max.y - bb.Min.y), ImGuiInputTextFlags_AutoSelectAll, nullptr);
        if (edited && ImGui::IsItemDeactivatedAfterEdit()) {
            edited = false;
            if (validator) {
                std::string text(buffer);
                text = validator->process(text);
                strncpy(buffer, text.data(), std::min(text.size(), static_cast<size_t>(buf_size)));
                buffer[std::min(text.size(), static_cast<size_t>(buf_size) - 1)] = '\0';
            }
            if (callbacks.text_edited) {
                callbacks.text_edited();
            }
        }
    } else {
        // Render preview and label
        if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
        {
            if (g.LogEnabled)
                ImGui::LogSetNextTextDecoration("{", "}");
            ImGui::RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
        }
    }

    if (!popup_open)
        return false;

    g.NextWindowData.Flags = backup_next_window_data_flags;
    return ImGui::BeginComboPopup(popup_id, bb, flags);
    // clang-format on
}

ComboBox::ComboBox(const std::string& name) { set_item_name(name); }

ComboBox::ComboBox(std::initializer_list<std::string> initializer_list, const std::string& name)
    : ComboBox(name)
{
    m_items = initializer_list;
    set_current_index(0);
}

ComboBox::~ComboBox() {}

void ComboBox::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    {
        Imgui::ScopedStyleColors colors(
            {{ImGuiCol_FrameBg, ImColor(41, 41, 41)},
             {ImGuiCol_FrameBgHovered, ImColor(60, 60, 60)},
             {ImGuiCol_FrameBgActive, ImColor(60, 60, 60)}}
        );

        ImGui::SetCursorScreenPos(to_im(pos));

        const std::string id = "###" + m_item_name;
        if (YGBeginCombo(
                id.c_str(), m_current_label.c_str(), to_im(size), m_flags, m_editable,
                m_buffer.data(), m_buffer.size(), m_updated, m_validator.get(), m_callbacks
            )) {
            int index = 0;
            for (const std::string& item : std::as_const(m_items)) {
                if (ImGui::Selectable(item.c_str(), index == m_current_index)) {
                    set_current_index(index);
                    if (m_callbacks.selection_changed)
                        m_callbacks.selection_changed(index);
                }
                index++;
            }

            ImGui::EndCombo();
        }
    }
    render_item_end(pos, size);
}

ComboBox::Callbacks& ComboBox::callbacks()
{
    return m_callbacks;
}

const std::vector<std::string>& ComboBox::items() const { return m_items; }

void ComboBox::set_items(const std::vector<std::string>& items)
{
    if (m_items != items) {
        m_items = items;
        set_current_index(0);
    }
}

Vec2f ComboBox::get_item_size()
{
    return {50, ImGui::GetTextLineHeight() + GImGui->Style.FramePadding.y * 2.0f};
}

Validator* ComboBox::validator() const { return m_validator.get(); }

void ComboBox::set_validator(std::unique_ptr<Validator> validator)
{
    m_validator = std::move(validator);
}

ImGuiComboFlags ComboBox::flags() const { return m_flags; }

void ComboBox::set_flags(ImGuiComboFlags flags) { m_flags = flags; }

bool ComboBox::editable() const { return m_editable; }

void ComboBox::set_editable(bool editable) { m_editable = editable; }

std::string ComboBox::current_label() const
{
    return m_editable ? std::string(m_buffer.data()) : m_current_label;
}

int ComboBox::current_index() const { return m_current_index; }

void ComboBox::set_current_index(int current_index)
{
    if (m_items.empty()) {
        m_current_index = -1;
        m_current_label = "";
        std::fill(m_buffer.begin(), m_buffer.end(), 0);
    } else {
        m_current_index = std::clamp(current_index, 0, static_cast<int>(m_items.size()) - 1);
        m_current_label = m_items.at(m_current_index);
        strcpy(m_buffer.data(), m_current_label.c_str());
    }
}

} // namespace Slic3r::App::Yoga
