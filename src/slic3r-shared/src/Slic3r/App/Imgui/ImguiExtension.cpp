
#include "Slic3r/App/Imgui/DoubleSlider.hpp"
#include "Slic3r/Assert.hpp"

#include <libslic3r/Color.hpp>

#include <boost/nowide/convert.hpp>

namespace Slic3r::App::Imgui {

static constexpr float TWO_PI = 2.0f * float(IM_PI);

void UnifiedWindowStyle::push()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::SetNextWindowBgAlpha(DEFAULT_WINDOW_BG_ALPHA);
}

void UnifiedWindowStyle::pop()
{
    ImGui::PopStyleVar(2);
}

void draw_hexagon(const ImVec2& center, float radius, ImU32 col, float start_angle, float rounding)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    float a_min = start_angle;
    float a_max = start_angle + TWO_PI;

    if (rounding <= 0)
        window->DrawList->PathArcTo(center, radius, a_min, a_max, 6);
    else {
        float a_delta = IM_PI / 4.0f;
        radius -= rounding;

        for (int i = 0; i <= 6; i++) {
            float a = a_min + ((float)i / 6.0f) * (a_max - a_min);
            if (a >= TWO_PI)
                a -= TWO_PI;
            ImVec2 pos = ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius);
            window->DrawList->PathArcTo(pos, rounding, a - a_delta, a + a_delta, 5);
        }
    }
    window->DrawList->PathFillConvex(col);
}

void tooltip(const char* label, float wrap_width)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6.0f, 6.0f });
    ImGui::SetNextWindowBgAlpha(DEFAULT_WINDOW_BG_ALPHA);
    ImGui::BeginTooltip();
    if (wrap_width > 0.0f) ImGui::PushTextWrapPos(wrap_width);
    ImGui::Text("%s", label);
    if (wrap_width > 0.0f) ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
    ImGui::PopStyleVar(2);
}

void tooltip(const std::string& label, float wrap_width)
{
    tooltip(label.c_str(), wrap_width);
}

void item_tooltip(const char* label, float wrap_width)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
        tooltip(label, wrap_width);
}

void item_tooltip(const std::string& label, float wrap_width)
{
    item_tooltip(label.c_str(), wrap_width);
}

static bool IsRootOfOpenMenuSet()
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) || (window->Flags & ImGuiWindowFlags_ChildMenu))
        return false;

    // Initially we used 'upper_popup->OpenParentId == window->IDStack.back()' to differentiate multiple menu sets from each others
    // (e.g. inside menu bar vs loose menu items) based on parent ID.
    // This would however prevent the use of e.g. PushID() user code submitting menus.
    // Previously this worked between popup and a first child menu because the first child menu always had the _ChildWindow flag,
    // making hovering on parent popup possible while first child menu was focused - but this was generally a bug with other side effects.
    // Instead we don't treat Popup specifically (in order to consistently support menu features in them), maybe the first child menu of a Popup
    // doesn't have the _ChildWindow flag, and we rely on this IsRootOfOpenMenuSet() check to allow hovering between root window/popup and first child menu.
    // In the end, lack of ID check made it so we could no longer differentiate between separate menu sets. To compensate for that, we at least check parent window nav layer.
    // This fixes the most common case of menu opening on hover when moving between window content and menu bar. Multiple different menu sets in same nav layer would still
    // open on hover, but that should be a lesser problem, because if such menus are close in proximity in window content then it won't feel weird and if they are far apart
    // it likely won't be a problem anyone runs into.
    const ImGuiPopupData* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];
    if (window->DC.NavLayerCurrent != upper_popup->ParentNavLayer)
        return false;
    return upper_popup->Window && (upper_popup->Window->Flags & ImGuiWindowFlags_ChildMenu) && ImGui::IsWindowChildOf(upper_popup->Window, window, true);
}

// see as reference: bool ImGui::MenuItemEx() in imgui_widgets.cpp
bool menu_item_with_icon(const char* label, const char* shortcut, ImU32 icon_color /* = 0*/, bool selected /* = false*/, bool enabled /* = true*/)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiStyle& style = g.Style;
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // See BeginMenuEx() for comments about this.
    bool menuset_is_open = IsRootOfOpenMenuSet();
    if (menuset_is_open)
        ImGui::PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);

    // We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
    // but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
    bool pressed = false;
    ImGui::PushID(label);
    if (!enabled)
        ImGui::BeginDisabled();

    // We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
    ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover;
    ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal) {
        DEBUG_ASSERT(false); // not implemented yet
        //// Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
        //// Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
        //float w = label_size.x;
        //window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * 0.5f);
        //ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        //pressed = ImGui::Selectable("", selected, selectable_flags, ImVec2(w, 0.0f));
        //ImGui::PopStyleVar();
        //if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)
        //  ImGui::RenderText(text_pos, label);
        //window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    }
    else {
        // Menu item inside a vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.
        float icon_w = (icon_color == 0) ? 0.0f : ImGui::GetTextLineHeight();
        float icon_size = (icon_w > 0.0f) ? icon_w + style.ItemInnerSpacing.x : 0.0f;
        float shortcut_w = (shortcut && shortcut[0]) ? ImGui::CalcTextSize(shortcut, nullptr).x : 0.0f;
        float checkmark_w = selected ? IM_TRUNC(g.FontSize * 1.20f) : 0.0f;
        float min_w = window->DC.MenuColumns.DeclColumns(icon_size, label_size.x, shortcut_w, checkmark_w); // Feedback for next frame
        float stretch_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x - min_w);
        unsigned int spacing_counter = 0;
        if (icon_w > 0.0f)       ++spacing_counter;
        if (shortcut != nullptr) ++spacing_counter;
        if (selected)            ++spacing_counter;
        ImVec2 item_size(label_size.x + icon_w + shortcut_w + checkmark_w + float(spacing_counter) * style.FramePadding.x, 0.0f);
        pressed = ImGui::Selectable("", false, selectable_flags | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y));
        if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible) {
            if (icon_w > 0.0f)
                ImGui::RenderFrame(pos, pos + ImVec2(icon_w, icon_w), icon_color);

             ImGui::RenderText(pos + ImVec2(offsets->OffsetLabel, 0.0f), label);

            if (shortcut_w > 0.0f) {
                ImGui::PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
                ImGui::RenderText(pos + ImVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, NULL, false);
                ImGui::PopStyleColor();
            }
            if (selected) {
                ImGui::RenderCheckMark(window->DrawList, pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f),
                    ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), g.FontSize * 0.866f);
            }
        }
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
    if (!enabled)
        ImGui::EndDisabled();
    ImGui::PopID();
    if (menuset_is_open)
        ImGui::PopItemFlag();

    return pressed;
}

void icon_image(wchar_t icon, const ImVec2& size)
{
    ImFont* font = ImGui::GetFont();
    float h = ImGui::GetTextLineHeight();
    ImVec2 rect = size;
    if (rect.x == 0.0f) rect.x = h;
    if (rect.y == 0.0f) rect.y = h;
    const ImFontGlyph* glyph = font->FindGlyph(icon);
    if (glyph != nullptr)
        ImGui::Image(font->ContainerAtlas->TexID, rect, { glyph->U0, glyph->V0 }, { glyph->U1, glyph->V1 });
}

bool icon_button(wchar_t icon, const ImVec2& size, const std::string& id)
{
    ImFont* font = ImGui::GetFont();
    float h = ImGui::GetTextLineHeight();
    ImVec2 rect = size;
    if (rect.x == 0.0f) rect.x = h;
    if (rect.y == 0.0f) rect.y = h;
    const ImFontGlyph* glyph = font->FindGlyph(icon);
    return (glyph != nullptr) ? ImGui::ImageButton(("##btn" + id).c_str(), font->ContainerAtlas->TexID, rect, { glyph->U0, glyph->V0 }, { glyph->U1, glyph->V1 }) : false;
}

ImU32 to_ImU32(const ColorRGBA& color)
{
    return IM_COL32(color.r_uchar(), color.g_uchar(), color.b_uchar(), color.a_uchar());
}

ImU32 to_ImU32(const ColorRGB& color, uint8_t alpha)
{
    return IM_COL32(color.r_uchar(), color.g_uchar(), color.b_uchar(), alpha);
}

} // namespace Slic3r::App::Imgui
