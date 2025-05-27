#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/ObjectList.hpp"
#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/SidebarPrint.hpp"

#define MAIN_WITH_SPLITTERS 1

namespace Slic3r::App {

namespace Scene {
class IToolGizmo;
}

namespace Yoga {
class Toolbar;
class ToolbarButton;
class Dialog;
} // namespace Yoga

enum class ToolbarID
{
    Top = 0,
    Middle,
    Bottom
};

class AbstractRenderLayout
{
public:
    using Vec2f = Yoga::Vec2f;

    static void set_our_style_colors();

    AbstractRenderLayout(
        std::unique_ptr<ObjectList> object_list,
        std::unique_ptr<CubeView> cube_view,
        std::unique_ptr<SidebarBed> sidebar_bed,
        std::unique_ptr<SidebarPrint> sidebar_print
    );
    virtual ~AbstractRenderLayout();
    AbstractRenderLayout(const AbstractRenderLayout& other) = delete;
    AbstractRenderLayout& operator=(const AbstractRenderLayout& other) = delete;

    virtual void init();

    void render(Vec2f size);

    Vec2f win_padding() const;
    Vec2f frame_padding() const;

    Yoga::ToolbarButton* add_toolbar_item(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        const std::string& shortcut,
        Yoga::AbstractButton::Callbacks callbacks
    );
    Yoga::ToolbarButton* add_toolbar_item_checkable(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        const std::string& shortcut,
        Yoga::AbstractButton::Callbacks callbacks,
        bool checked = false
    );
    Yoga::ToolbarButton* add_toolbar_item_gizmo(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        const std::string& shortcut,
        Yoga::AbstractButton::Callbacks callbacks,
        Scene::IToolGizmo* tool
    );
    Yoga::ToolbarButton* add_toolbar_item_panel(
        ToolbarID id,
        Render::Icon icon,
        const std::string& tooltip,
        const std::string& shortcut,
        Yoga::AbstractButton::Callbacks callbacks,
        Yoga::Item* panel
    );

    Yoga::Toolbar* top_toolbar() const;
    Yoga::Toolbar* middle_toolbar() const;
    Yoga::Toolbar* bottom_toolbar() const;

    /**
     * @brief The reason this method exists is because toolbars are justified in "Space around" mode
     * hiding one would break this design and therefore we need to juggle around dummy items so
     * Layout will stay the same
     * */
    void set_bottom_toolbar_visible(bool visible);

    void set_sidebars_visible(bool visible);

protected:
    virtual void init_left_column();
    virtual void init_middle_column();
    virtual void init_right_column();

    Yoga::Toolbar* find_toolbar(ToolbarID id) const;
    void update_toolbar_tooltip();
    void update_sidebar_visibility();

private:
    void init_toolbar_column();

protected:
    Yoga::Item m_layout_main;
    Yoga::Item* m_layout_left_column = nullptr;
    Yoga::Item* m_layout_center_row = nullptr;
    Yoga::Item* m_layout_right_column = nullptr;

    Yoga::Item* m_layout_left_toolbar_column = nullptr;
    Yoga::Item* m_layout_middle_column = nullptr;

    Yoga::Toolbar* m_top_toolbar = nullptr;
    Yoga::Toolbar* m_middle_toolbar = nullptr;
    Yoga::Toolbar* m_bottom_toolbar = nullptr;
    Yoga::Item* m_bottom_dummy_toolbar = nullptr;

    struct SidebarPanel {
        Yoga::Item* panel = nullptr;
        bool last_visible = false;
        bool visible = false;
    };
    std::map<Yoga::ToolbarButton*, SidebarPanel> m_sidebar_panels;
    bool m_sidebars_visible = true;

    // Inserted from render module
    Yoga::Passthrough<ObjectList> m_object_list;
    Yoga::Passthrough<CubeView> m_cube_view;
    Yoga::Passthrough<SidebarBed> m_sidebar_bed;
    Yoga::Passthrough<SidebarPrint> m_sidebar_print;
};

} // namespace Slic3r::App
