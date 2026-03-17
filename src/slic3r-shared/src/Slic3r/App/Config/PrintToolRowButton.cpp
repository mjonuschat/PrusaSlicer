///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/PrintToolRowButton.hpp"

#include "Slic3r/Biz/PrintToolItem.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/ConfigItemPreview.hpp"
#include "Slic3r/App/Config/ConfigItemUtils.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PrintToolRowButton::PrintToolRowButton()
{
    set_content_orientation(Orientation::Horizontal);
    set_content_align_items(YGAlignCenter);
    set_content_justify_content(YGJustifyFlexStart);
    set_background_color(ImGui::GetStyleColorVec4(ImGuiCol_Button));
    set_background_color_checked(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    set_allow_overlap(true);
    set_checkable(true);

    m_icon_caret = emplace_back<Icon>(Render::Icon::CaretUp);
    m_icon_caret->set_width(22);
    m_icon_caret->set_height(22);
    m_label = emplace_back<Text>(std::string{});
    m_label->set_flex_grow(1);
    m_label->set_wrap_mode(Text::WrapMode::WrapElide);

    const ImColor warning_color = ImColor(223, 93, 45);

    m_compatibility_rule_rect = emplace_back<Rectangle>();
    m_compatibility_rule_rect->set_fill(IM_COL32_BLACK_TRANS);
    m_compatibility_rule_rect->set_border_color(warning_color);
    m_compatibility_rule_rect->set_gap(5);
    m_compatibility_rule_rect->set_padding(4);
    m_compatibility_rule_label =
        m_compatibility_rule_rect->emplace_back<Text>(std::string{"conflict resolved"});
    m_compatibility_rule_label->set_text_color(warning_color);
    m_compatibility_rule_label->set_visible(false);
    m_config_item_preview = m_compatibility_rule_rect->emplace_back<ConfigItemPreview>();
    m_compatibility_rule_rect->set_flex_shrink(0.f);

    m_per_extruder_label = emplace_back<Text>(Biz::_u8L("Per tool"), Render::ImguiFontType::Italic);
    m_per_extruder_label->set_visible(false);

    m_tooltip->set_text_wrap(true);
    m_tooltip->content_item()->set_width(350);
}

void PrintToolRowButton::update_data(const Biz::PrintToolItem* print_tool_item)
{
    ASSERT(print_tool_item);
    m_last_print_tool_item = print_tool_item;
    bool show_preview      = false;

    if (print_tool_item->print_item->def().require_compatibility_rule) {
        show_preview = true;
        m_config_item_preview
            ->set_data(*print_tool_item->print_item, print_tool_item->value.first, false);
    } else if (!print_tool_item->shared_context.extruder_candidates.empty()) {
        // Maybe the tool values are same
        const Domain::ConfigValue& first_value =
            print_tool_item->tool_value(*print_tool_item->shared_context.extruder_candidates.cbegin());
        show_preview = std::all_of(
            print_tool_item->shared_context.extruder_candidates.cbegin(),
            print_tool_item->shared_context.extruder_candidates.cend(),
            [&](unsigned extruder) { return print_tool_item->tool_value(extruder) == first_value; }
        );
        m_config_item_preview->set_data(*print_tool_item->print_item, first_value, false);
    } else {
        const std::size_t tool_count = print_tool_item->tool_overrides.size();
        const Domain::ConfigValue& first_value =
            print_tool_item->tool_value(0);
        bool all_same = true;
        for (std::size_t extruder_id{}; extruder_id < tool_count; ++extruder_id) {
            if (print_tool_item->tool_value(extruder_id) != first_value) {
                all_same = false;
            }
        }
        show_preview = all_same;
        m_config_item_preview->set_data(*print_tool_item->print_item, first_value, false);
    }

    m_config_item_preview->set_visible(show_preview);
    m_per_extruder_label->set_visible(!show_preview);

    update_rule_visibility();

    set_tooltip(ConfigItemUtils::config_item_tooltip(*print_tool_item->print_item));
    m_label->set_text(Biz::_u8(print_tool_item->print_item->def().label));
}

void PrintToolRowButton::checked_updated_internal()
{
    m_icon_caret->set_icon(checked() ? Render::Icon::CaretDown : Render::Icon::CaretUp);
    set_draw_flags(checked() ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll);
}

void PrintToolRowButton::update_rule_visibility()
{
    bool rule_visible =
        m_last_print_tool_item->tool_overrides.size() > 1 && m_last_print_tool_item->value.second;
    m_compatibility_rule_label->set_visible(rule_visible);
    m_compatibility_rule_rect->set_border_width(rule_visible ? 1 : 0);
}

} // namespace Slic3r::App
