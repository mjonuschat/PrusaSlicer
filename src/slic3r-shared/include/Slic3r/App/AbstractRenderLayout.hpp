#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#define MAIN_WITH_SPLITTERS 1

namespace Slic3r::App {

namespace Yoga {
class Toolbar;
class ToolbarButton;
}

class ObjectList;
class CubeView;
class SidebarPrint;
class SidebarBed;

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

    AbstractRenderLayout(
        ObjectList* object_list,
        CubeView* cube_view,
        SidebarBed* sidebar_bed,
        SidebarPrint* sidebar_print
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
        wchar_t icon,
        const std::string& tooltip,
        const std::string& shortcut,
        Yoga::AbstractButton::Callbacks callbacks
    );

    Yoga::Toolbar* top_toolbar() const;
    Yoga::Toolbar* middle_toolbar() const;
    Yoga::Toolbar* bottom_toolbar() const;

    /**
     * @brief The reason this method exists is because toolbars are justified in "Space around" mode
     * hiding one would break this design and therefore we need to juggle around dummy items so Layout
     * will stay the same
     * */
    void set_bottom_toolbar_visible(bool visible);

protected:
    virtual void init_left_column();
    virtual void init_middle_column();
    virtual void init_right_column();

    Yoga::Toolbar* find_toolbar(ToolbarID id) const;
    void update_toolbar_tooltip();

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

    // Inserted from render module
    ObjectList* m_object_list = nullptr;
    CubeView* m_cube_view = nullptr;
    SidebarBed* m_sidebar_bed = nullptr;
    SidebarPrint* m_sidebar_print = nullptr;
};

} // namespace Slic3r::App
