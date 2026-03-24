#pragma once

#include "Slic3r/App/Yoga/RootItem.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/TopBar.hpp"
#include "Slic3r/App/Yoga/ToolbarSwitchButton.hpp"
#include "Slic3r/App/ObjectListWindow.hpp"
#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/PopNotification/PopNotificationListView.hpp"
#include "Slic3r/App/SidebarObject.hpp"
#include "Slic3r/App/PreferencesDialog.hpp"

namespace Slic3r::App {

class SidebarStackLayout;

namespace Scene {
class IToolGizmo;
} // namespace Scene

namespace Yoga {
class Toolbar;
class Dialog;
class SplitLayout;
} // namespace Yoga

enum class ToolbarID
{
    Left = 0,
    Middle,
    Right
};

class AbstractRenderLayout
{
public:
    using Vec2f = Yoga::Vec2f;

    AbstractRenderLayout(
        Navigator& navigator,
        std::unique_ptr<TopBar> top_bar,
        std::unique_ptr<PreferencesDialog> preferences_dialog,
        std::unique_ptr<ObjectListWindow> object_list,
        std::unique_ptr<CubeView> cube_view,
        std::unique_ptr<PopNotification::PopNotificationListView> pop_notification_list_view,
        std::unique_ptr<SidebarBed> sidebar_bed,
        std::unique_ptr<SidebarPrint> sidebar_print,
        std::unique_ptr<SidebarObject> sidebar_object
    );
    virtual ~AbstractRenderLayout();
    AbstractRenderLayout(const AbstractRenderLayout& other)            = delete;
    AbstractRenderLayout& operator=(const AbstractRenderLayout& other) = delete;

    virtual void init();

    void render(Vec2f size);

    Yoga::ToolbarButton* add_toolbar_item(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip
    );
    Yoga::ToolbarButton* add_toolbar_item_checkable(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        bool checked = false
    );
    Yoga::ToolbarButton* add_toolbar_item_gizmo(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        Scene::IToolGizmo* tool
    );
    Yoga::ToolbarSwitchButton* add_toolbar_item_switch(
        ToolbarID id,
        Render::Icon icon,
        const std::string& label,
        const std::string& tooltip,
        Yoga::ToolbarSwitchButton::SwitchPosition switch_position
    );

    Yoga::Toolbar* left_toolbar() const;
    Yoga::Toolbar* middle_toolbar() const;
    Yoga::Toolbar* right_toolbar() const;

    SidebarStackLayout* sidebar_stack_layout() const;

    void set_sidebars_visible(bool visible);

    void save_column_sizes();
    void load_column_sizes();

protected:
    virtual void init_left_column();
    virtual void init_middle_column();
    virtual void init_right_column();

    Yoga::Toolbar* find_toolbar(ToolbarID id) const;

    void update_cube_view_position();
    void update_left_separator_enable();

private:
    void init_toolbar_row();

protected:
    Navigator& m_navigator;

    Yoga::RootItem m_layout_main;
    Yoga::SplitLayout* m_layout_main_bottom = nullptr;
    Yoga::Item* m_layout_left_column        = nullptr;
    Yoga::Item* m_layout_center_row         = nullptr;
    Yoga::Item* m_layout_right_column       = nullptr;

    Yoga::Item* m_layout_middle_toolbar_row = nullptr;
    Yoga::Item* m_layout_middle_column      = nullptr;
    Yoga::Item* m_layout_scene_row          = nullptr;

    SidebarStackLayout* m_layout_sidebar_stack_layout = nullptr;

    // we are moving CubeView between Horizontal and Vertical layouts, wrapper is needed
    Yoga::Item* m_cube_view_wrapper = nullptr;

    Yoga::Toolbar* m_left_toolbar   = nullptr;
    Yoga::Toolbar* m_middle_toolbar = nullptr;
    Yoga::Toolbar* m_right_toolbar  = nullptr;

    bool m_sidebars_visible = true;

    // Inserted from render module
    Yoga::Passthrough<TopBar> m_top_bar;
    Yoga::Passthrough<ObjectListWindow> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<PopNotification::PopNotificationListView> m_pop_notification_list_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
    Yoga::Passthrough<SidebarObject> m_sidebar_object;
    Yoga::Passthrough<PreferencesDialog> m_preferences_dialog;
};

} // namespace Slic3r::App
