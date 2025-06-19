#pragma once

#include <Slic3r/Domain/SelectionId.hpp>

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/ProjectButton.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App::Platform {
class AbstractRenderModule;
}

namespace Slic3r::App {

namespace Yoga {
class ProjectButton;
class LayoutButton;
class Rectangle;
} // namespace Yoga

class TopBar : public Yoga::Window
{
public:
    explicit TopBar(
        Biz::ProjectInteractor* project_interactor, Platform::AbstractRenderModule* render_module
    );

private:
    void add_load_project_btn(Item* parent);
    void add_save_project_btn(Item* parent);
    void add_show_ui_btn(Item* parent);

    void add_new_project_btn(Item* parent);
    void add_expander_btn(Item* parent);

private:
    using ProjectButtonListView = Yoga::ListView<
        Yoga::ProjectButton,
        Domain::SelectionId,
        Yoga::ViewFactory<Yoga::ProjectButton, Domain::SelectionId, Biz::ProjectInteractor&>>;

    ProjectButtonListView* m_list_view{nullptr};

    Yoga::LayoutButton* m_load_btn{nullptr};
    Yoga::LayoutButton* m_save_btn{nullptr};
    Yoga::LayoutButton* m_show_ui_btn{nullptr};

    Yoga::LayoutButton* m_new_btn{nullptr};
    Yoga::LayoutButton* m_expand_btn{nullptr};

    Yoga::Rectangle* m_search{nullptr};

    Biz::ProjectInteractor* m_project_interactor{nullptr};
    Platform::AbstractRenderModule* m_render_module{nullptr};
};

} // namespace Slic3r::App
