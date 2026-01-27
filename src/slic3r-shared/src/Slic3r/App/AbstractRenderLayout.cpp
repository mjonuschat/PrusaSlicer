#include "Slic3r/App/AbstractRenderLayout.hpp"

#include <imgui_internal.h>

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Yoga/ToolbarSwitchButton.hpp"
#include "Slic3r/App/Yoga/SplitLayout.hpp"
#include "Slic3r/App/SidebarStackLayout.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/Navigator.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

SidebarStackLayout* AbstractRenderLayout::sidebar_stack_layout() const
{
    return m_layout_sidebar_stack_layout;
}

void AbstractRenderLayout::save_column_sizes()
{
    AppServices::instance().app_config().set<double>(
        "layout_main_left_column_width",
        static_cast<double>(m_layout_left_column->width())
    );
    AppServices::instance().app_config().set<double>(
        "layout_main_right_column_width",
        static_cast<double>(m_layout_right_column->width())
    );
}

void AbstractRenderLayout::load_column_sizes()
{
    if (m_layout_left_column && m_layout_right_column) {
        m_layout_left_column->set_width(
            AppServices::instance().app_config().get<double>("layout_main_left_column_width")
        );
        m_layout_right_column->set_width(
            AppServices::instance().app_config().get<double>("layout_main_right_column_width")
        );
        m_layout_main_bottom->invalidate();
    }
}

void AbstractRenderLayout::update_cube_view_position()
{
    // When sidebars are visible and object list is collapsed
    // move cube_view to left_column
    // otherwise put it in the scene_row
    enum class Location
    {
        LeftColumn,
        SceneRow
    };

    const Location current_location = m_layout_scene_row->index_of(m_cube_view_wrapper).has_value() ?
        Location::SceneRow :
        Location::LeftColumn;
    const Location target_location  = m_sidebars_visible && m_object_list->collapsed() ?
         Location::LeftColumn :
         Location::SceneRow;

    if (current_location == target_location) {
        return;
    }

    if (target_location == Location::LeftColumn) {
        m_layout_left_column->append(m_layout_scene_row->remove(m_cube_view_wrapper));
        m_cube_view_wrapper->set_self_align(YGAlign::YGAlignFlexStart);
        m_cube_view_wrapper->set_flex_grow(1);
    } else {
        m_layout_scene_row->prepend(m_layout_left_column->remove(m_cube_view_wrapper));
        m_cube_view_wrapper->set_self_align(YGAlign::YGAlignFlexEnd);
        m_cube_view_wrapper->set_flex_grow(0);
    }
}

void AbstractRenderLayout::update_left_separator_enable()
{
    // everytime a collapse state change in any of the CollapsibleWindow
    // located in Left column is done we need to update
    // SplitLayout left column separator enable

    bool enabled = false;
    for (size_t child_index = 0; child_index < m_layout_left_column->object_count(); ++child_index) {
        CollapsibleWindow* window =
            dynamic_cast<CollapsibleWindow*>(m_layout_left_column->get_item(child_index));
        if (window && !window->collapsed()) {
            enabled = true;
            break;
        }
    }

    m_layout_main_bottom->set_separator_enable(0, enabled);
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip
)
{
    Toolbar* toolbar = find_toolbar(id);
    ASSERT(toolbar);

    toolbar->set_visible(true);

    ToolbarButton* button = toolbar->emplace_back<ToolbarButton>(icon, tooltip);

    return button;
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item_checkable(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    bool checked
)
{
    ToolbarButton* button = add_toolbar_item(id, icon, tooltip);
    ASSERT(button);

    button->set_checked(checked);
    button->set_checkable(true);

    return button;
}

ToolbarButton* AbstractRenderLayout::add_toolbar_item_gizmo(
    ToolbarID id,
    Render::Icon icon,
    const std::string& tooltip,
    Scene::IToolGizmo* tool
)
{
    ToolbarButton* button = add_toolbar_item(id, icon, tooltip);
    ASSERT(button);

    return button;
}

ToolbarSwitchButton* AbstractRenderLayout::add_toolbar_item_switch(
    ToolbarID id,
    Render::Icon icon,
    const std::string& label,
    const std::string& tooltip,
    ToolbarSwitchButton::SwitchPosition switch_position
)
{
    Toolbar* toolbar = find_toolbar(id);
    ASSERT(toolbar);

    toolbar->set_visible(true);

    ToolbarSwitchButton* switch_button =
        toolbar->emplace_back<ToolbarSwitchButton>(switch_position, icon, label, tooltip);

    return switch_button;
}

Toolbar* AbstractRenderLayout::find_toolbar(ToolbarID id) const
{
    switch (id) {
    case ToolbarID::Left:
        return m_left_toolbar;
    case ToolbarID::Middle:
        return m_middle_toolbar;
    case ToolbarID::Right:
        return m_right_toolbar;
    }
    return nullptr;
}

Toolbar* AbstractRenderLayout::right_toolbar() const
{
    return m_right_toolbar;
}

Toolbar* AbstractRenderLayout::middle_toolbar() const
{
    return m_middle_toolbar;
}

Toolbar* AbstractRenderLayout::left_toolbar() const
{
    return m_left_toolbar;
}

void AbstractRenderLayout::set_sidebars_visible(bool visible)
{
    if (m_sidebars_visible != visible) {
        m_sidebars_visible = visible;

        m_layout_main_bottom->set_visible_child(m_layout_right_column, m_sidebars_visible);
        m_layout_main_bottom->set_visible_child(m_layout_left_column, m_sidebars_visible);

        update_cube_view_position();
    }
}

void AbstractRenderLayout::init()
{
    m_layout_main.set_padding(0);
    m_layout_main.set_gap(0);
    m_layout_main.set_orientation(Orientation::Vertical);

    m_layout_main.append(m_top_bar.release());

    m_layout_main_bottom = m_layout_main.emplace_back<SplitLayout>();
    m_layout_main_bottom->set_orientation(Orientation::Horizontal);
    m_layout_main_bottom->set_padding(Paddings(5.f, 5.f));
    m_layout_main_bottom->set_flex_grow(1.0);

    init_left_column();

    init_middle_column();

    init_right_column();

    m_layout_main.append(m_preferences_dialog.release());
    m_preferences_dialog->attach_to_center();
}

void AbstractRenderLayout::init_left_column()
{
    m_layout_left_column = m_layout_main_bottom->emplace_back<Item>();
    m_layout_left_column->set_min_size({280.f, 0.f});
    m_layout_left_column->set_justify_content(YGJustifyFlexStart);
    m_layout_left_column->set_width(
        AppServices::instance().app_config().get<double>("layout_main_left_column_width")
    );
    m_layout_left_column->set_orientation(Orientation::Vertical);
    m_layout_left_column->set_gap(5);

    m_object_list->collapsible_window_callbacks().collapsed_changed = [this](bool collapsed)
    {
        update_cube_view_position();
        update_left_separator_enable();
        m_navigator.set_object_list_collapsed(collapsed);
    };
    m_layout_left_column->append(m_object_list.release());
}

void AbstractRenderLayout::init_middle_column()
{
    m_layout_center_row = m_layout_main_bottom->emplace_back<Item>();
    m_layout_center_row->set_min_size({340, YGUndefined});
    m_layout_center_row->set_orientation(Orientation::Horizontal);
    m_layout_center_row->set_gap(5);
    m_layout_main_bottom->set_flex_child(m_layout_center_row, true);

    m_layout_middle_column = m_layout_center_row->emplace_back<Item>();
    m_layout_middle_column->set_orientation(Orientation::Vertical);
    m_layout_middle_column->set_gap(5);
    m_layout_middle_column->set_flex_grow(1);
    m_layout_middle_column->set_object_name("MiddleColumn");

    init_toolbar_row();

    // Row for CubeView (left) and Notification column (right)
    m_layout_scene_row = m_layout_middle_column->emplace_back<Yoga::Item>();
    m_layout_scene_row->set_orientation(Orientation::Horizontal);
    m_layout_scene_row->set_flex_grow(1);

    m_cube_view_wrapper = m_layout_scene_row->emplace_back<Item>();
    m_cube_view_wrapper->set_self_align(YGAlignFlexEnd);
    m_cube_view_wrapper->set_justify_content(YGJustifyFlexEnd);
    m_cube_view_wrapper->set_align_items(YGAlignFlexStart);
    m_cube_view_wrapper->set_orientation(Orientation::Vertical);
    m_cube_view_wrapper->append(m_cube_view.release());

    Item* spacer = m_layout_scene_row->emplace_back<Item>();
    spacer->set_flex_grow(1);

    m_layout_scene_row->append(m_pop_notification_list_view.release());
    m_pop_notification_list_view->set_orientation(Orientation::Vertical);
    m_pop_notification_list_view->set_max_size({400.f, YGUndefined});
    m_pop_notification_list_view->set_flex_grow(1);
    m_pop_notification_list_view->set_margin(10.);
    m_pop_notification_list_view->set_self_align(YGAlignFlexEnd);
    m_pop_notification_list_view->set_source_list(
        &AppServices::instance().pop_notification_center().source_list()
    );
}

void AbstractRenderLayout::init_right_column()
{
    m_layout_right_column = m_layout_main_bottom->emplace_back<Item>();
    m_layout_right_column->set_orientation(Orientation::Vertical);
    m_layout_right_column->set_gap(5);
    m_layout_right_column->set_width(
        AppServices::instance().app_config().get<double>("layout_main_right_column_width")
    );
    m_layout_right_column->set_min_size({240, YGUndefined});

    m_layout_sidebar_stack_layout = m_layout_right_column->emplace_back<SidebarStackLayout>();
    m_layout_sidebar_stack_layout->set_orientation(Orientation::Vertical);
    m_layout_sidebar_stack_layout->set_flex_grow(1);

    ItemPtr sidebar_bed_print = std::make_unique<Item>();
    sidebar_bed_print->set_orientation(Orientation::Vertical);
    sidebar_bed_print->set_gap(5);
    sidebar_bed_print->set_flex_grow(1);
    sidebar_bed_print->append(m_sidebar_bed.release());
    sidebar_bed_print->append(m_sidebar_print.release());
    sidebar_bed_print->append(m_sidebar_physical.release());
    m_layout_sidebar_stack_layout->insert_item(
        SidebarStackLayout::ItemType::Bed,
        std::move(sidebar_bed_print)
    );

    m_layout_sidebar_stack_layout->insert_item(
        SidebarStackLayout::ItemType::Object,
        m_sidebar_object.release()
    );
}

void AbstractRenderLayout::init_toolbar_row()
{
    constexpr float button_size = 40.f;

    m_layout_middle_toolbar_row = m_layout_middle_column->emplace_back<Item>();
    m_layout_middle_toolbar_row->set_orientation(Orientation::Horizontal);
    m_layout_middle_toolbar_row->set_gap(5);
    m_layout_middle_toolbar_row->set_justify_content(YGJustify::YGJustifyCenter);
    m_layout_middle_toolbar_row->set_z(1); // Increaze Z so toolbars can be on top of double sliders

    m_left_toolbar = m_layout_middle_toolbar_row->emplace_back<Toolbar>("TopToolbar");
    m_left_toolbar->set_button_width(button_size);
    m_left_toolbar->set_button_height(button_size);
    m_left_toolbar->set_orientation(Orientation::Horizontal);
    m_left_toolbar->set_flex_shrink(0);
    m_left_toolbar->set_visible(false);

    m_middle_toolbar = m_layout_middle_toolbar_row->emplace_back<Toolbar>("MiddleToolbar");
    m_middle_toolbar->set_button_width(button_size);
    m_middle_toolbar->set_button_height(button_size);
    m_middle_toolbar->set_orientation(Orientation::Horizontal);
    m_middle_toolbar->set_collapsible(true);
    m_middle_toolbar->set_visible(false);

    m_right_toolbar = m_layout_middle_toolbar_row->emplace_back<Toolbar>("BottomToolbar");
    m_right_toolbar->set_button_width(button_size);
    m_right_toolbar->set_button_height(button_size);
    m_right_toolbar->set_orientation(Orientation::Horizontal);
    m_right_toolbar->set_flex_shrink(0);
    m_right_toolbar->set_visible(false);
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

    ~SetOurStyleVars()
    {
        ImGui::PopStyleVar(m_vars_cnt);
    }

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
    Navigator& navigator,
    std::unique_ptr<TopBar> top_bar,
    std::unique_ptr<PreferencesDialog> preferences_dialog,
    std::unique_ptr<ObjectListWindow> object_list,
    std::unique_ptr<CubeView> cube_view,
    std::unique_ptr<PopNotification::PopNotificationListView> pop_notification_list_view,
    std::unique_ptr<SidebarBed> sidebar_bed,
    std::unique_ptr<SidebarPrint> sidebar_print,
    std::unique_ptr<SidebarObject> sidebar_object,
    std::unique_ptr<SidebarPhysical> sidebar_physical
) :
    m_navigator(navigator),
    m_top_bar(std::move(top_bar)),
    m_object_list(std::move(object_list)),
    m_cube_view(std::move(cube_view)),
    m_pop_notification_list_view(std::move(pop_notification_list_view)),
    m_sidebar_bed(std::move(sidebar_bed)),
    m_sidebar_print(std::move(sidebar_print)),
    m_sidebar_object(std::move(sidebar_object)),
    m_sidebar_physical(std::move(sidebar_physical)),
    m_preferences_dialog(std::move(preferences_dialog))
{}

AbstractRenderLayout::~AbstractRenderLayout()
{
    save_column_sizes();
}

void AbstractRenderLayout::render(Vec2f size)
{
    SetOurStyleVars our_vars;
    m_layout_main.render({}, size);
}

} // namespace Slic3r::App
