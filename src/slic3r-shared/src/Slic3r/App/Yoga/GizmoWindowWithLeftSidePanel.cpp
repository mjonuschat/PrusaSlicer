#include "Slic3r/App/Yoga/GizmoWindowWithLeftSidePanel.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

namespace Slic3r::App::Yoga {

const constexpr float DIALOG_CONTENT_PADDING   = 20;
const constexpr float DIALOG_HEADER_HEIGHT     = 36.f;
const constexpr float SIDE_PANEL_HEADER_HEIGHT = 40.f;
const constexpr float SIDE_PANEL_WIDTH         = 80.f;
const ImColor HEADER_LABEL_COLOR               = ImColor(184, 184, 184);
const Paddings DIALOG_HEADER_PADDING           = Paddings(20, 10, 10, 10);

GizmoWindowWithLeftSidePanel::GizmoWindowWithLeftSidePanel(
    const std::string& title,
    Render::Icon icon
) :
    GizmoWindow()
{
    this->set_orientation(Orientation::Horizontal);
    this->set_gap(0);
    this->set_padding(0);
    this->set_flex_grow(1);
    this->set_flags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    // Side panel on the left (full height, fixed width).
    m_side_panel = this->emplace_back<Item>();
    m_side_panel->set_flex_shrink(0);
    m_side_panel->set_width(SIDE_PANEL_WIDTH);
    m_side_panel->set_orientation(Orientation::Vertical);

    // Background rectangle for the side panel (with rounded corners on the left).
    m_side_panel_background = m_side_panel->emplace_back<Rectangle>();
    m_side_panel_background->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
    m_side_panel_background->set_flags(ImDrawFlags_RoundCornersLeft);
    m_side_panel_background->set_rounding(5.0f);
    m_side_panel_background->set_flex_grow(1);
    m_side_panel_background->set_orientation(Orientation::Vertical);

    // Header row inside the side panel.
    m_side_panel_header = m_side_panel_background->emplace_back<Item>();
    m_side_panel_header->set_height(SIDE_PANEL_HEADER_HEIGHT);
    m_side_panel_header->set_flex_shrink(0);
    m_side_panel_header->set_justify_content(YGJustifyCenter);
    m_side_panel_header->set_align_items(YGAlignCenter);

    // Header label text.
    m_side_panel_header_title = m_side_panel_header->emplace_back<Text>("");
    m_side_panel_header_title->set_text_color(HEADER_LABEL_COLOR);

    // Content section.
    m_side_panel_content = m_side_panel_background->emplace_back<Item>();
    m_side_panel_content->set_flex_grow(1);
    m_side_panel_content->set_padding(Paddings(0, 0, 0, 5));

    // Dialog section on the right.
    Item* dialog_section = this->emplace_back<Item>();
    dialog_section->set_orientation(Orientation::Vertical);
    dialog_section->set_flex_grow(1);

    // Header row inside the dialog section.
    m_top_row = dialog_section->emplace_back<Item>();
    m_top_row->set_max_size({YGUndefined, DIALOG_HEADER_HEIGHT});
    m_top_row->set_flex_shrink(0);

    Rectangle* buttons_rect = m_top_row->emplace_back<Rectangle>();
    buttons_rect->set_align_items(YGAlignCenter);
    buttons_rect->set_padding(DIALOG_HEADER_PADDING);
    buttons_rect->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
    buttons_rect->set_flex_grow(1);
    buttons_rect->set_flags(ImDrawFlags_RoundCornersTopRight);

    Icon* header_icon = buttons_rect->emplace_back<Icon>(icon);
    header_icon->set_margin(Margins{0, 0, 3, 0});
    header_icon->set_width(20);
    header_icon->set_height(20);

    Text* title_text = buttons_rect->emplace_back<Text>(title);
    title_text->set_font_type(Render::ImguiFontType::Bold);

    Item* spacer = buttons_rect->emplace_back<Item>();
    spacer->set_flex_grow(1);

    m_revert_button = buttons_rect->emplace_back<LayoutButton>("", Render::Icon::UndoGizmo);
    m_revert_button->set_min_size({22, 22});
    m_revert_button->callbacks().action = [this]
    {
        if (m_gizmo_callback.revert_requested) {
            m_gizmo_callback.revert_requested();
        }
    };
    m_revert_button->set_visible(false);
    m_revert_button->set_margin(Margins(10.f, 0.f));

    m_close_button = buttons_rect->emplace_back<LayoutButton>("", Render::Icon::CloseGizmo);
    m_close_button->set_min_size({22, 22});
    m_close_button->callbacks().action = [this]
    {
        if (m_gizmo_callback.close_requested) {
            m_gizmo_callback.close_requested();
        }
    };

    this->add_separator(dialog_section);

    m_content = dialog_section->emplace_back<Item>();
    m_content->set_padding(DIALOG_CONTENT_PADDING);
}

Item* GizmoWindowWithLeftSidePanel::side_panel_content() const
{
    return m_side_panel_content;
}

void GizmoWindowWithLeftSidePanel::set_side_panel_background_color(ImColor color)
{
    if (m_side_panel_background == nullptr) {
        return;
    }

    m_side_panel_background->set_fill(color);
}

void GizmoWindowWithLeftSidePanel::set_side_panel_header_title(const std::string& title)
{
    if (m_side_panel_header_title == nullptr) {
        return;
    }

    m_side_panel_header_title->set_text(title);
}

} // namespace Slic3r::App::Yoga
