#include "Slic3r/App/AbstractRenderLayout.hpp"

#include <imgui_internal.h>

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

Vec2f AbstractRenderLayout::win_padding() const
{
    return Vec2f(GImGui->Style.WindowPadding.x, GImGui->Style.WindowPadding.y);
}

Vec2f AbstractRenderLayout::frame_padding() const
{
    return Vec2f(GImGui->Style.FramePadding.x, GImGui->Style.FramePadding.y);
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    const std::string& shortcut,
    AbstractButton::Callbacks callbacks
)
{
    Toolbar* toolbar = find_toolbar(id);
    ASSERT(toolbar);

    Passthrough<ToolbarButton> button = Passthrough(std::make_unique<ToolbarButton>(icon, tooltip));
    toolbar->append(button.release());
    button->set_shortcut(shortcut);
    button->callbacks() = callbacks;

    return button.get();
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item_checkable(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    const std::string& shortcut,
    AbstractButton::Callbacks callbacks,
    bool checked
)
{
    ToolbarButton* button = add_toolbar_item(id, icon, tooltip, shortcut, callbacks);
    ASSERT(button);

    button->set_checked(checked);
    button->set_checkable(true);

    return button;
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item_gizmo(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    const std::string& shortcut,
    Yoga::AbstractButton::Callbacks callbacks,
    Scene::IToolGizmo* tool
)
{
    ToolbarButton* button = add_toolbar_item(id, icon, tooltip, shortcut, callbacks);
    ASSERT(button);

    std::unique_ptr<Dialog> dialog_uniq = tool->unlaod_ui_dialog();
    Dialog* dialog = dialog_uniq.get();
    if (dialog) {
        button->append(std::move(dialog_uniq));
        dialog->set_visible(false);
        button->callbacks().checked_changed = [dialog](bool checked) {
            dialog->set_visible(checked);
        };
    }

    return button;
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item_panel(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    const std::string& shortcut,
    Yoga::AbstractButton::Callbacks callbacks,
    Yoga::Item* panel
)
{
    ASSERT(panel);

    ToolbarButton* button =
        add_toolbar_item(id, icon, tooltip, shortcut, callbacks);
    ASSERT(button);

    button->set_checked(panel->is_visible());

    button->callbacks().action = [this, button] {
        SidebarPanel& sidebar = m_sidebar_panels[button];
        sidebar.visible = !sidebar.visible;
        update_sidebar_visibility();
    };

    m_sidebar_panels[button] = {panel, panel->is_visible(), panel->is_visible()};

    return button;
}

Toolbar* AbstractRenderLayout::find_toolbar(ToolbarID id) const
{
    switch (id) {
    case ToolbarID::Top:
        return m_top_toolbar;
    case ToolbarID::Middle:
        return m_middle_toolbar;
    case ToolbarID::Bottom:
        return m_bottom_toolbar;
    }
    return nullptr;
}

void AbstractRenderLayout::update_toolbar_tooltip()
{
    // if any toolbar is hovered and also subtoolbar is now opened;
    bool show_tooltips =
        (m_top_toolbar->hovered() || m_middle_toolbar->hovered() || m_bottom_toolbar->hovered()) &&
        !m_top_toolbar->any_subtoolbar_opened() && !m_middle_toolbar->any_subtoolbar_opened() &&
        !m_bottom_toolbar->any_subtoolbar_opened();
    m_top_toolbar->set_show_tooltips(show_tooltips);
    m_middle_toolbar->set_show_tooltips(show_tooltips);
    m_bottom_toolbar->set_show_tooltips(show_tooltips);
}

void AbstractRenderLayout::update_sidebar_visibility()
{
    m_layout_right_column->set_visible(m_sidebars_visible);

    bool left_sidebar_visible = m_sidebars_visible;
    for (auto& [button, panel] : m_sidebar_panels) {
        button->set_checked(panel.visible);
        panel.panel->set_visible(panel.visible);
        left_sidebar_visible |= panel.visible;
    }
    m_layout_left_column->set_visible(left_sidebar_visible);
}

void AbstractRenderLayout::set_bottom_toolbar_visible(bool visible)
{
    m_bottom_toolbar->set_visible(visible);
    m_bottom_dummy_toolbar->set_visible(!visible);
}

Toolbar* AbstractRenderLayout::bottom_toolbar() const { return m_bottom_toolbar; }

Toolbar* AbstractRenderLayout::middle_toolbar() const { return m_middle_toolbar; }

Toolbar* AbstractRenderLayout::top_toolbar() const { return m_top_toolbar; }

void AbstractRenderLayout::set_sidebars_visible(bool visible)
{
    if (m_sidebars_visible != visible) {
        m_sidebars_visible = visible;

        if (m_sidebars_visible) {
            for (auto& [button, panel] : m_sidebar_panels) {
                panel.visible = panel.visible || panel.last_visible;
            }
        } else {
            for (auto& [button, panel] : m_sidebar_panels) {
                panel.last_visible = panel.panel->is_visible();
                panel.visible = false;
            }
        }
        update_sidebar_visibility();
    }
}

void AbstractRenderLayout::synchronize_topbar()
{
    m_top_bar->synchronize();
}

void AbstractRenderLayout::init()
{
    m_layout_main.set_padding(0);
    m_layout_main.set_gap(0);
    m_layout_main.set_orientation(Orientation::Vertical);

    m_layout_main.append(m_top_bar.release());

    m_layout_main_bottom = m_layout_main.emplace_back<Item>();
    m_layout_main_bottom->set_gap(5);
    m_layout_main_bottom->set_orientation(Orientation::Horizontal);
    m_layout_main_bottom->set_padding(Paddings(frame_padding()));
    m_layout_main_bottom->set_flex_grow(1.0);

    init_left_column();

    init_middle_column();

    init_right_column();
}

void AbstractRenderLayout::init_left_column()
{
    m_layout_left_column = m_layout_main_bottom->emplace_back<Item>();
    m_layout_left_column->set_orientation(Orientation::Vertical);
    m_layout_left_column->set_gap(5);

    m_object_list->set_flex_grow(1.);
    m_layout_left_column->append(m_object_list.release());
}

void AbstractRenderLayout::init_middle_column()
{
    m_layout_center_row = m_layout_main_bottom->emplace_back<Item>();
    m_layout_center_row->set_orientation(Orientation::Horizontal);
    m_layout_center_row->set_gap(5);
    m_layout_center_row->set_flex_grow(1.);

    init_toolbar_column();

    m_layout_middle_column = m_layout_center_row->emplace_back<Item>();
    m_layout_middle_column->set_orientation(Orientation::Vertical);
    m_layout_middle_column->set_gap(5);
    m_layout_middle_column->set_flex_grow(1);

    m_layout_middle_column->append(m_cube_view.release());
    m_cube_view->set_self_align(YGAlignFlexEnd);
}

void AbstractRenderLayout::init_right_column()
{
    m_layout_right_column = m_layout_main_bottom->emplace_back<Item>();
    m_layout_right_column->set_orientation(Orientation::Vertical);
    m_layout_right_column->set_gap(5);
    m_layout_right_column->set_min_size({280.f, 0});

    m_layout_right_column->append(m_sidebar_bed.release());

    m_layout_right_column->append(m_sidebar_print.release());
}

void AbstractRenderLayout::init_toolbar_column()
{
    constexpr float min_tt_size = 46.f; //**/ 30.f;
    constexpr float max_tt_size = 46.f;

    m_layout_left_toolbar_column = m_layout_center_row->emplace_back<Item>();
    m_layout_left_toolbar_column->set_orientation(Orientation::Vertical);
    m_layout_left_toolbar_column->set_gap(5);
    m_layout_left_toolbar_column->set_justify_content(YGJustify::YGJustifySpaceBetween);
    m_layout_left_toolbar_column->set_z(1
    ); // Increaze Z so toolbars can be on top of double sliders

    m_top_toolbar = m_layout_left_toolbar_column->emplace_back<Toolbar>("top_toolbar");
    m_top_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_top_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_top_toolbar->set_self_align(YGAlign::YGAlignFlexStart);
    m_top_toolbar->set_orientation(Orientation::Vertical);

    m_middle_toolbar = m_layout_left_toolbar_column->emplace_back<Toolbar>("middle_toolbar");
    m_middle_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_middle_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_middle_toolbar->set_self_align(YGAlign::YGAlignCenter);
    m_middle_toolbar->set_orientation(Orientation::Vertical);
    m_middle_toolbar->set_collapsible(true);

    m_bottom_toolbar = m_layout_left_toolbar_column->emplace_back<Toolbar>("bottom_toolbar");
    m_bottom_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_bottom_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_bottom_toolbar->set_self_align(YGAlign::YGAlignFlexEnd);
    m_bottom_toolbar->set_orientation(Orientation::Vertical);

    m_bottom_dummy_toolbar = m_layout_left_toolbar_column->emplace_back<Item>();
    m_bottom_dummy_toolbar->set_visible(false);

    m_top_toolbar->callbacks().hovered_changed = [this]() { update_toolbar_tooltip(); };
    m_middle_toolbar->callbacks().hovered_changed = [this]() { update_toolbar_tooltip(); };
    m_bottom_toolbar->callbacks().hovered_changed = [this]() { update_toolbar_tooltip(); };
    m_top_toolbar->callbacks().subtoolbar_opened = [this]() { update_toolbar_tooltip(); };
    m_middle_toolbar->callbacks().subtoolbar_opened = [this]() { update_toolbar_tooltip(); };
    m_bottom_toolbar->callbacks().subtoolbar_opened = [this]() { update_toolbar_tooltip(); };
}

void AbstractRenderLayout::set_our_style_colors()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.106f, 0.106f, 0.106f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = colors[ImGuiCol_WindowBg];// ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.21f, 0.29f, 0.46f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected] =
        ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected] =
        ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] =
        ImVec4(0.31f, 0.31f, 0.35f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] =
        ImVec4(0.23f, 0.23f, 0.25f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

struct SetOurStyleVars
{
    SetOurStyleVars()
    {
        PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
        PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 6.f));
        PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 0.f));
    }

    ~SetOurStyleVars() { ImGui::PopStyleVar(m_vars_cnt); }

private:
    void PushStyleVar(ImGuiStyleVar idx, float val)
    {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }
    void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
    {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }

    size_t m_vars_cnt{0};
};

AbstractRenderLayout::AbstractRenderLayout(
    std::unique_ptr<TopBar> top_bar,
    std::unique_ptr<ObjectList> object_list,
    std::unique_ptr<CubeView> cube_view,
    std::unique_ptr<SidebarBed> sidebar_bed,
    std::unique_ptr<SidebarPrint> sidebar_print
)
    : m_top_bar(std::move(top_bar))
    , m_object_list(std::move(object_list))
    , m_cube_view(std::move(cube_view))
    , m_sidebar_bed(std::move(sidebar_bed))
    , m_sidebar_print(std::move(sidebar_print))
{}

AbstractRenderLayout::~AbstractRenderLayout() {}

void AbstractRenderLayout::render(Vec2f size)
{
    SetOurStyleVars our_vars;
    m_layout_main.render({}, size);
}

} // namespace Slic3r::App
