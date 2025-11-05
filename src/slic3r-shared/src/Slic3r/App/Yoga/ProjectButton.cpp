#include "Slic3r/App/Yoga/ProjectButton.hpp"

#include "Slic3r/App/Yoga/ProjectButtonBackground.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <fmt/format.h>
#include <imgui/imgui_internal.h>

using namespace Slic3r::Biz;

namespace Slic3r::App::Yoga {

ProjectButton::ProjectButton(
    size_t index,
    const Domain::SelectionId& data,
    Biz::ProjectInteractor& project_interactor
) :
    Biz::DataObserver<Domain::SelectionId>(index, data),
    m_project_interactor(project_interactor)
{
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);

    set_allow_overlap(true);
    set_flex_shrink(0);

    m_background = emplace_back<ProjectButtonBackground>();
    m_background->set_margin(Margins(0, -1));
    m_background->set_padding({20.f, 5.f});
    m_background->set_rounding(0.f);
    m_background->set_gap(7.f);
    m_background->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);

    set_tooltip_position(Position::Bottom);

    m_label = m_background->emplace_back<Text>("");
    m_label->set_self_align(YGAlignCenter);

    m_cross = m_background->emplace_back<LayoutButton>("", Render::Icon::TopBarCross);
    m_cross->set_width(20);
    m_cross->set_height(20);
    m_cross->set_self_align(YGAlignCenter);
    m_cross->set_flex_shrink(0);
    m_cross->set_background_color(IM_COL32_BLACK_TRANS);

    on_data_update();
}

bool ProjectButton::is_cross_hovered() const
{
    return m_cross->hovered();
}

bool ProjectButton::is_selected()
{
    return m_selected;
}

void ProjectButton::set_selected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        m_background->set_mode(
            selected ? ProjectButtonBackground::Border : ProjectButtonBackground::FilledRect
        );
        m_label->set_text_color(GImGui->Style.Colors[selected ? ImGuiCol_Text : ImGuiCol_TextDisabled]);
        m_label->set_font_type(selected ? Render::ImguiFontType::Bold : Render::ImguiFontType::Regular);
    }
}

void ProjectButton::on_selected_project_changed(size_t index)
{
    set_selected(*m_state == index);
}

void ProjectButton::hovered_updated_internal()
{
    if (m_selected) {
        return;
    }

    m_background->set_mode(
        hovered() ? ProjectButtonBackground::Border : ProjectButtonBackground::FilledRect
    );
}

void ProjectButton::on_data_update()
{
    const boost::filesystem::path proj_path(m_project_interactor.get_project_name(*m_state));
    std::string btn_label;
    std::string btn_tooltip;
    if (proj_path.empty()) {
        std::string new_project = _u8L("New Project");
        if (*m_state) {
            new_project += fmt::format(" ({})", *m_state);
        }
        btn_label   = new_project;
        btn_tooltip = new_project;
    } else {
        btn_label   = proj_path.filename().string();
        btn_tooltip = proj_path.string();
    }

    set_tooltip(btn_tooltip);
    m_label->set_text(btn_label);

    m_cross->callbacks().action = [this]() {
        m_project_interactor.remove_project(*m_state);
    };

    callbacks().action = [this]() {
        // Ignore action, if cross button was clicked or if button is already selected
        if (m_cross->hovered() || m_selected) {
            return;
        }
        // select related project
        m_project_interactor.select_project(*m_state);
    };

    set_selected(m_project_interactor.selected_project_id() == *m_state);
}

void ProjectButton::on_view_will_be_removed()
{
    m_project_interactor.remove_listener<Biz::ISelectedProjectChangedListener>(this);
}

} // namespace Slic3r::App::Yoga
