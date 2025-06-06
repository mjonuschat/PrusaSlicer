#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App::Platform {
class AbstractRenderModule;
}

namespace Slic3r::App {

namespace Yoga{
    class ProjectButton;
    class LayoutButton;
    class Item;
    class Rectangle;
}   

class TopBar : public Yoga::Window {
public:
    explicit TopBar(Biz::ProjectInteractor* project_interactor, Platform::AbstractRenderModule* render_module);

    void synchronize();

private:
    void add_project_button(size_t project_id, bool select = false);
    void select_project_button(Yoga::ProjectButton* button);
    void remove_project_button(Yoga::ProjectButton* button);

    void add_load_project_btn(Item* parent);
    void add_save_project_btn(Item* parent);
    void add_show_ui_btn(Item* parent);

    void init_from_project_interactor();

    void add_new_project_btn(Item* parent);
    void add_expander_btn(Item* parent);

private:

    Yoga::LayoutButton* m_load_btn{ nullptr };
    Yoga::LayoutButton* m_save_btn{ nullptr };
    Yoga::LayoutButton* m_show_ui_btn{ nullptr };

    Yoga::Item* m_buttons_wrapper{ nullptr };

    Yoga::LayoutButton* m_new_btn{ nullptr };
    Yoga::LayoutButton* m_expand_btn{ nullptr };

    Yoga::Rectangle* m_search{ nullptr };

    Biz::ProjectInteractor* m_project_interactor{ nullptr };
    Platform::AbstractRenderModule* m_render_module{ nullptr };
};

} // namespace Slic3r::App::Yoga