#include "Slic3r/App/Yoga/ProjectButton.hpp"
#include "Slic3r/App/Yoga/ProjectButtonBackground.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga {

ProjectButton::ProjectButton(const std::string& name, size_t project_id)
: AbstractButton(""),
m_project_id(project_id)
{
    m_background = emplace_back<ProjectButtonBackground>();
    m_background->set_padding({ 20.f, 5.f });
    m_background->set_rounding(0.f);
    m_background->set_gap(7.f);
    m_background->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);

    m_label = m_background->emplace_back<Text>(name);
    m_label->set_self_align(YGAlignCenter);

    m_cross = m_background->emplace_back<LayoutButton>("", Render::Icon::TopBarCross);
    m_cross->set_max_size({ 20.f, 20.f });
    m_cross->set_self_align(YGAlignCenter);
    m_cross->set_background_color(IM_COL32_BLACK_TRANS);

    set_tooltip_position(Position::Bottom);
}

std::function<void()>& ProjectButton::on_cross() 
{
    return m_cross->callbacks().action;
}

bool ProjectButton::is_cross_hovered() const
{
    return m_cross->hovered();
}

size_t ProjectButton::project_id() const { return m_project_id; }

bool ProjectButton::is_selected() { return m_selected; }

void ProjectButton::set_selected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        m_background->set_mode(selected ? ProjectButtonBackground::Border : ProjectButtonBackground::FilledRect);
        m_label->set_text_color(GImGui->Style.Colors[selected ? ImGuiCol_Text : ImGuiCol_TextDisabled]);
        m_label->set_font_type(selected ? Render::ImguiFontType::Bold: Render::ImguiFontType::Regular);
    }
}

void ProjectButton::hovered_updated_internal()
{
    if (m_selected)
        return;
    m_background->set_mode(hovered() ? ProjectButtonBackground::Border : ProjectButtonBackground::FilledRect);
}

} // namespace Slic3r::App::Yoga