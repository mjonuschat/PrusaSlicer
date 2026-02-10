#pragma once

#include <Slic3r/Domain/SelectionId.hpp>

#include <Slic3r/Biz/Platform/ListenerScope.hpp>

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include "Slic3r/App/MenuCommandRegistrar.hpp"

namespace Slic3r::App::Platform {
class AbstractRenderModule;
} // namespace Slic3r::App::Platform

namespace Slic3r::App {

namespace Yoga {
class ProjectButton;
class LayoutButton;
class Rectangle;
class ScrollArea;
class Menu;
} // namespace Yoga

struct ThumbnailStore;
class SearchBar;
class Navigator;

class TopBar : public Yoga::Window, public Biz::ISelectedProjectChangedListener
{
public:
    TopBar(
        Biz::ProjectInteractor* project_interactor,
        Platform::AbstractRenderModule* render_module,
        ThumbnailStore& thumbnail_store,
        Navigator& navigator
    );

    void on_selected_project_changed(size_t index) override;

    void focus_search();

private:
    void add_save_project_btn(Item* parent);
    void add_show_ui_btn(Item* parent);

    void add_menu_btns(Item* parent);

private:
    using ProjectButtonListView = Yoga::ListView<
        Yoga::ProjectButton,
        Domain::SelectionId,
        Yoga::ViewFactory<Yoga::ProjectButton, Domain::SelectionId, Biz::ProjectInteractor&>,
        Yoga::ScrollArea>;

    Biz::ListenerScope<Biz::ISelectedProjectChangedListener, Biz::ProjectInteractor, TopBar>
        m_selected_project_changed_listener_scope;

    ProjectButtonListView* m_list_view{nullptr};

    Yoga::LayoutButton* m_main_menu_btn{nullptr};
    Yoga::LayoutButton* m_file_menu_btn{nullptr};
    Yoga::LayoutButton* m_save_btn{nullptr};
    Yoga::LayoutButton* m_show_ui_btn{nullptr};

    Yoga::Menu* m_main_menu{nullptr};
    Yoga::Menu* m_file_menu{nullptr};

    SearchBar* m_search_bar{nullptr};

    Biz::ProjectInteractor& m_project_interactor;
    Platform::AbstractRenderModule* m_render_module{nullptr};
    Navigator& m_navigator;

    MenuCommandRegistrar m_menu_command_registrar;
};

} // namespace Slic3r::App
