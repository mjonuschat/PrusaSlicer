#include "Slic3r/App/ColorDropdown.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/ImGuiUtils.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include <algorithm>

#include <imgui_internal.h>

#include <algorithm>

namespace Slic3r::App::Yoga {

ColorMenuItem::ColorMenuItem(
    const std::string& label,
    const Domain::ColorRGBA& color,
    bool selectable,
    bool dropdown_indicator,
    bool hollow,
    std::optional<std::string> index
) :
    m_dropdown_indicator(dropdown_indicator)
{
    set_min_height(24_fpx);
    set_width_percent(100);
    set_content_padding(
        m_dropdown_indicator ? Yoga::Paddings{8_fpx, 3_fpx, 24_fpx, 3_fpx} : Yoga::Paddings{8_fpx, 3_fpx}
    );
    set_content_align_items(YGAlignCenter);
    set_content_justify_content(YGJustifyFlexStart);
    set_background_color(Platform::Color::ButtonTransparent);
    if (selectable) {
        set_background_color_checked(Platform::Color::Button);
        set_checkable(true);
    }

    m_swatch = emplace_back<Yoga::Circle>();
    auto text{m_swatch->emplace_back<Text>("")};
    text->set_font_type(Render::ImguiFontType::Bold);
    const ImColor imgui_color{color.r_uchar(), color.g_uchar(), color.b_uchar(), color.a_uchar()};
    text->set_text_color(Imgui::contrast_color(imgui_color));
    m_swatch->set_justify_content(YGJustifyCenter);
    m_swatch->set_align_items(YGAlignFlexStart);

    auto* spacer = emplace_back<Yoga::Item>();
    spacer->set_flex_grow(1);

    set_entry(label, color, hollow, index);
}

void ColorMenuItem::set_entry(
    const std::string& label,
    const Domain::ColorRGBA& color,
    const bool hollow,
    std::optional<std::string> index
)
{
    m_label = label;
    const ImColor imgui_color{color.r_uchar(), color.g_uchar(), color.b_uchar(), color.a_uchar()};
    if (hollow) {
        m_swatch->set_width(10_fpx);
        m_swatch->set_height(10_fpx);
        const Unit margin{3_fpx};
        m_swatch->set_margin({margin, margin, margin, margin});
        m_swatch->set_fill(m_theme->color_imgui(Platform::Color::Transparent));
        m_swatch->set_border_width(1);
        m_swatch->set_border_color(imgui_color);
    } else {
        m_swatch->set_width(index ? 16_fpx : 14_fpx);
        m_swatch->set_height(index ? 16_fpx : 14_fpx);
        m_swatch->set_margin(0);
        m_swatch->set_fill(imgui_color);
        m_swatch->set_padding(0);
        m_swatch->set_border_width(0);
        m_swatch->set_border_color(m_theme->color_imgui(Platform::Color::Transparent));
    }

    ASSERT(m_swatch->items().size() == 1);
    auto text{dynamic_cast<Text*>(m_swatch->get_item(0))};
    ASSERT(text);
    text->set_text(index ? *index : "");
}

static float fpx(float value) {
    constexpr float imgui_scale_factor{18.0f / 14.0f};
    return value * imgui_scale_factor;
}

void ColorMenuItem::render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size)
{
    Yoga::RectangleButton::render(pos, size);

    const ImRect bb{to_im(pos), to_im(pos + size)};

    const ImVec2 swatch_pos{to_im(m_swatch->get_global_pos())};
    const float swatch_right{swatch_pos.x + m_swatch->width()};

    const float text_left{swatch_right + fpx(6)};
    const float right_padding{fpx(4)};
    float text_right{bb.Max.x - right_padding};

    if (!m_dropdown_indicator) {
        ImRect text_rect{ImVec2(text_left, bb.Min.y), ImVec2(text_right, bb.Max.y)};
        ImGui::RenderTextClipped(
            text_rect.Min,
            text_rect.Max,
            m_label.c_str(),
            nullptr,
            nullptr,
            ImVec2(0.f, 0.5f),
            &text_rect
        );
        return;
    }

    ImDrawList* draw_list{ImGui::GetWindowDrawList()};
    if (draw_list == nullptr) {
        return;
    }

    const ImGuiStyle& style{GImGui->Style};
    const float arrow_size{ImGui::GetFrameHeight()};
    const float value_x2{ImMax(bb.Min.x, bb.Max.x - arrow_size)};

    text_right = value_x2 - style.FramePadding.x;
    ImRect text_rect{ImVec2(text_left, bb.Min.y), ImVec2(text_right, bb.Max.y)};
    ImGui::RenderTextClipped(
        text_rect.Min,
        text_rect.Max,
        m_label.c_str(),
        nullptr,
        nullptr,
        ImVec2(0.f, 0.5f),
        &text_rect
    );

    const ImU32 text_col{ImGui::GetColorU32(hovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled)};
    if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x) {
        const float w{arrow_size - 2.f * style.FramePadding.x};
        const float h{GImGui->FontSize};
        Yoga::YGRenderArrow(
            draw_list,
            ImVec2(value_x2 + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
            ImVec2(w, h),
            text_col,
            ImGuiDir_Down,
            1.0f
        );
    }
}

static std::vector<std::pair<Domain::ColorRGBA, std::string>> get_material_colors(
    const Biz::ProjectInteractor& project_interactor
)
{
    const Slic3r::Biz::Preset::PresetInteractor& preset_interactor{
        project_interactor.preset_interactor()
    };
    const Biz::ProjectSettingsInteractor& settings_interactor{
        project_interactor.project_settings_interactor()
    };
    Domain::SelectionId config_container_id{project_interactor.selected_config_container_id()};

    const auto rgb_colors{settings_interactor.get_colors(config_container_id)};

    std::vector<Domain::ColorRGBA> colors;
    colors.reserve(rgb_colors.size());
    std::ranges::transform(
        rgb_colors,
        std::back_inserter(colors),
        [](const Domain::ColorRGB& color)
        { return Domain::ColorRGBA{color.r(), color.g(), color.b(), 1.0f}; }
    );

    std::vector<std::string> names;
    const auto& selected{preset_interactor.selected_printer_preset()};
    names.reserve(selected.materials.size());
    for (const auto& material : selected.materials) {
        names.push_back(std::string{material.short_name()});
    }

    std::vector<std::pair<Domain::ColorRGBA, std::string>> result{};

    std::size_t count{std::min(names.size(), colors.size())};
    for (std::size_t i{}; i < count; ++i) {
        result.push_back({colors[i], names[i]});
    }
    return result;
}


ColorDropdown::ColorDropdown(Biz::ProjectInteractor& project_interactor, bool with_default, bool with_numbers) :
    m_project_interactor{project_interactor},
    m_with_default{with_default},
    m_with_numbers{with_numbers}
{
    m_project_interactor.project_settings_interactor().add_listener<Biz::IColorsChangedListener>(
        this
    );
    m_project_interactor.preset_interactor().add_listener<Biz::Preset::IPresetChangedListener>(
        this
    );

    set_object_name("ColorDropdown");

    m_trigger = emplace_back<ColorMenuItem>(std::string{}, Domain::ColorRGBA{}, true, true);
    m_trigger->set_flex_grow(1);
    m_trigger->set_background_color(Platform::Color::Button);
    m_trigger->set_background_color_checked(
        m_theme->color_imgui(Platform::Color::Button, Platform::ColorGroup::Hovered)
    );

    m_popup = m_trigger->emplace_back<Yoga::ContextPopup>("MMPaintingColorDropdownPopup");
    m_popup->set_orientation(Yoga::Orientation::Vertical);
    m_popup->set_width_percent(100);
    m_popup->set_padding({2_fpx, 2_fpx});
    m_popup->set_gap(2_fpx);
    m_popup->set_offset(2);
    m_popup->set_position(Yoga::Position::Bottom);

    m_trigger->callbacks().action = [this]()
    {
        if (m_popup->opened()) {
            m_popup->close();
        } else {
            m_popup->open();
        }
    };

    m_popup->callbacks().opened = [this]() { m_trigger->set_checked(true); };
    m_popup->callbacks().closed = [this]() { m_trigger->set_checked(false); };

    set_items(get_material_colors(m_project_interactor));
}

ColorDropdown::~ColorDropdown()
{
    m_project_interactor.project_settings_interactor().remove_listener<Biz::IColorsChangedListener>(
        this
    );
    m_project_interactor.preset_interactor().remove_listener<Biz::Preset::IPresetChangedListener>(
        this
    );
}

void ColorDropdown::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& colors
)
{
    set_items(get_material_colors(m_project_interactor));
}

void ColorDropdown::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    set_items(get_material_colors(m_project_interactor));
}

void ColorDropdown::set_items(
    const std::vector<std::pair<Domain::ColorRGBA, std::string>>& material_colors
)
{
    if (material_colors.empty()) {
        return;
    }

    m_material_colors = material_colors;

    rebuild_popup_items();
    update_trigger_label();
}

void ColorDropdown::set_current_index(std::optional<std::size_t> index)
{
    if (m_material_colors.empty()) {
        return;
    }

    if (!index) {
        m_current_index = std::nullopt;
        update_trigger_label();
        return;
    }

    const std::size_t items_count{
        m_material_colors.size() + (m_with_default ? std::size_t{1} : std::size_t{0})
    };

    m_current_index = *index;

    if (m_current_index >= items_count) {
        rebuild_popup_items();
    }

    for (std::size_t i{}; i < m_popup_items.size(); ++i) {
        m_popup_items[i]->set_checked(i == m_current_index);
    }
    update_trigger_label();
}

void ColorDropdown::set_current_index_internal(std::size_t index)
{
    const std::size_t items_count{
        m_material_colors.size() + (m_with_default ? std::size_t{1} : std::size_t{0})
    };

    ASSERT(index < items_count);
    m_current_index = index;
    for (std::size_t i{}; i < m_popup_items.size(); ++i) {
        m_popup_items[i]->set_checked(i == m_current_index);
    }
    update_trigger_label();
}

std::size_t ColorDropdown::current_index() const
{
    ASSERT(m_current_index);
    return *m_current_index;
}

void ColorDropdown::style_node()
{
    if (m_popup != nullptr && m_trigger != nullptr) {
        m_popup->set_width(m_trigger->width());
    }

    Yoga::Item::style_node();
}

struct Label
{
    Domain::ColorRGBA color;
    std::string name;
    bool hollow{};
    std::optional<std::string> index;
};

struct Label get_label(
    std::optional<std::size_t> index,
    bool with_default,
    bool with_numbers,
    const std::vector<std::pair<Domain::ColorRGBA, std::string>>& material_colors
)
{
    if (!index) {
        return {
            .color  = Domain::ColorRGBA::WHITE(),
            .name   = Biz::_u8L("Mixed"),
            .hollow = true,
        };
    }

    const Label unkown_label{
        .color = Domain::ColorRGBA::RED(),
        .name  = fmt::format(
            fmt::runtime(Biz::_u8L("Invalid extruder ({})")),
            with_default ? *index : *index + 1
        ),
        .hollow = true,
    };

    if (with_default) {
        if (index == 0) {
            return {
                .color  = Domain::ColorRGBA::WHITE(),
                .name   = Biz::_u8L("Default"),
                .hollow = true,
            };
        } else {
            ASSERT(index > 0);
            if (*index - 1 >= material_colors.size()) {
                return unkown_label;
            } else {
                const auto& [color, name]{material_colors[*index - 1]};
                return {
                    .color = color,
                    .name  = name,
                    .index = with_numbers ? std::optional{std::to_string(*index)} : std::nullopt
                };
            }
        }
    } else {
        if (index >= material_colors.size()) {
            return unkown_label;
        } else {
            const auto& [color, name]{material_colors[*index]};
            return {
                .color = color,
                .name  = name,
                .index = with_numbers ? std::optional{std::to_string(*index + 1)} : std::nullopt
            };
        }
    }
}

void ColorDropdown::rebuild_popup_items()
{
    while (!m_popup->items().empty()) {
        m_popup->remove(m_popup->items().back());
    }

    m_popup_items.clear();

    const std::size_t items_count{
        m_material_colors.size() + (m_with_default ? std::size_t{1} : std::size_t{0})
    };

    for (std::size_t index{}; index < items_count; ++index) {
        const auto& [color, name, hollow, index_string]{
            get_label(index, m_with_default, m_with_numbers, m_material_colors)
        };
        ColorMenuItem* item{
            m_popup->emplace_back<ColorMenuItem>(name, color, true, false, hollow, index_string)
        };
        item->set_checked(index == m_current_index);
        m_popup_items.push_back(item);
        item->callbacks().action = [this, index]()
        {
            set_current_index_internal(index);
            on_color_selected(index);
            m_popup->close();
        };
    }
}

void ColorDropdown::update_trigger_label()
{
    if (m_material_colors.empty()) {
        m_trigger->set_entry({}, Domain::ColorRGBA{});
        return;
    }
    const auto& [color, name, hollow, index]{
        get_label(m_current_index, m_with_default, m_with_numbers, m_material_colors)
    };
    m_trigger->set_entry(name, color, hollow, index);
}

} // namespace Slic3r::App::Yoga
