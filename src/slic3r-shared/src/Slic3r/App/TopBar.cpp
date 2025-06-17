#include "Slic3r/App/TopBar.hpp"
#include "Slic3r/App/Yoga/ProjectButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include <Slic3r/App/IDialogManager.hpp>
#include "Slic3r/App/I18N/I18N.hpp"
#include "imgui/imgui_internal.h"

#include <set>

//tmp include
#include "libslic3r/format.hpp"

namespace Slic3r::App {

using namespace Yoga;

TopBar::TopBar(
    Biz::ProjectInteractor* project_interactor, 
    Platform::AbstractRenderModule* render_module
): Window("top_bar"),
m_project_interactor(project_interactor),
m_render_module(render_module)
{
    Paddings paddings = padding();

    set_padding(0);
    set_alpha(0.f);
    set_rounding(0.f);
    set_gap(0.f);

    Rectangle* left_wrapper = emplace_back<Rectangle>();

    add_load_project_btn(left_wrapper);
    add_save_project_btn(left_wrapper);
    add_show_ui_btn(left_wrapper);

    m_buttons_wrapper = emplace_back<Item>();

    // add initial project(s)
    init_from_project_interactor();

    Rectangle* project_actions_wrapper = emplace_back<Rectangle>();
    project_actions_wrapper->set_flex_grow(1.);

    add_new_project_btn(project_actions_wrapper);
    add_expander_btn(project_actions_wrapper);

    Rectangle* right_wrapper = emplace_back<Rectangle>();
    m_search = right_wrapper->emplace_back<Rectangle>();
    m_search->set_min_size({ 200.f, YGUndefined });
    m_search->set_rounding(0.f);

    Text* shortcut = right_wrapper->emplace_back<Text>("Ctrl+F");
    shortcut->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    shortcut->set_self_align(YGAlignCenter);

    for (LayoutButton* btn : std::initializer_list<LayoutButton*> {
                            m_load_btn,
                            m_save_btn,
                            m_show_ui_btn,
                            m_new_btn,
        })
        btn->set_background_color(IM_COL32_BLACK_TRANS);

    for (Rectangle* wrapper : std::initializer_list<Rectangle*>{
                            left_wrapper,
                            right_wrapper,
                            project_actions_wrapper,
        }) {
        wrapper->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);
        wrapper->set_rounding(0.f);
        wrapper->set_gap(15.f);
        wrapper->set_padding(paddings);
    }
}

void TopBar::add_load_project_btn(Item* parent)
{
    m_load_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarLoad);
    m_load_btn->callbacks().action = [this]() {
        IDialogManager::FileCallback callback = [this](bool success, const boost::filesystem::path& file_path) {
            if (success) {
                size_t project_id = m_project_interactor->load_project(file_path.string());
                add_project_button(project_id, true);
            }
        };

        auto& dlg_manager = DialogManagerProvider::instance().get();
        dlg_manager.show_file_dialog(FileDialogType::Open, _u8L("Open Project"), "", "", "*.3mf", callback);
    };
}

void TopBar::add_save_project_btn(Item* parent)
{
    m_save_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarSave);
    m_save_btn->callbacks().action = [this]() {
        auto& dlg_manager = DialogManagerProvider::instance().get();
        dlg_manager.show_yesno_dialog(
            "DEVELOPER WARNING",
            "EXPORT TO 3MF IS NOT FINALIZED YET.\n\nThe exported project MUST NOT be shared publicly, "
            "it will not be compatible with both old PrusaSlicer and the finalized 3.0.0.\n\n"
            "Do you really want to export it?",
            [this](bool answer) {
                if (! answer)
                    return;
                const std::string& project_name = m_project_interactor->get_project_name(m_project_interactor->selected_project_id());
                if (true || project_name.empty()) { // The 'true' is here for the development phase - effectively it always "Saves as".
                    // Saving a new project - show file save dialog.
                    IDialogManager::FileCallback callback = [this](bool success, const boost::filesystem::path& file_path) {
                        if (success)
                            m_project_interactor->save_project(file_path.string());
                    };
                    auto& dlg_manager = DialogManagerProvider::instance().get();            
                    dlg_manager.show_file_dialog(FileDialogType::Open, _u8L("Save Project"), "", "", "*.3mf", callback);
                } else {
                    // Saving an existing project - just save.
                    m_project_interactor->save_project(project_name);
                }
            }
        );
    };
}

void TopBar::add_show_ui_btn(Item* parent)
{
    m_show_ui_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarShowUI);
    m_show_ui_btn->set_checkable(true);

    m_show_ui_btn->callbacks().action = [this]() {
        m_show_ui_btn->set_tooltip(m_show_ui_btn->checked() ? _u8L("Show sidebars") : _u8L("Hide sidebars"));
        // Propagate sidebars visibility into active RenderModule
        m_render_module->set_sidebars_visible(!m_show_ui_btn->checked());

        // ysTODO: save hide value into app_config
    };
}

void TopBar::init_from_project_interactor()
{
    ASSERT(m_buttons_wrapper->items().size() == 0);
    const auto& projects = m_project_interactor->workbench().projects();
    size_t selected_project_id = m_project_interactor->selected_project_id();
    for (const auto& [project_id, project] : projects) {
        add_project_button(project_id, selected_project_id == project_id);
    }
}

void TopBar::add_new_project_btn(Item* parent)
{
    m_new_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarPlus);
    m_new_btn->set_background_color(IM_COL32_BLACK_TRANS);
    m_new_btn->callbacks().action = [this]() {
        size_t project_id = m_project_interactor->new_project();
        add_project_button(project_id, true);
    };
}

void TopBar::add_expander_btn(Item* parent)
{
    m_expand_btn = parent->emplace_back<LayoutButton>("", Render::Icon::OpenArrow);
    m_expand_btn->set_background_color(IM_COL32_BLACK_TRANS);
    m_expand_btn->set_visible(false);
}

void TopBar::add_project_button(size_t project_id, bool select)
{
    const std::string proj_name = m_project_interactor->get_project_name(project_id);
    const std::string btn_label = proj_name.empty() ? format("%1% (%2%)", _u8L("New Project"), project_id) : proj_name;
    ProjectButton* proj_btn = m_buttons_wrapper->emplace_back<ProjectButton>(btn_label, project_id);

    proj_btn->callbacks().action = [this, proj_btn]()
    { 
        // Ignore action, if cross button was clicked or if button is already selected
        if (proj_btn->is_cross_hovered() || proj_btn->is_selected()) {
            return; 
        }
        // select related project
        select_project_button(proj_btn);
        m_project_interactor->select_project(proj_btn->project_id());
    };

    proj_btn->on_cross() = [proj_btn, this]() { remove_project_button(proj_btn); };

    if (select)
        select_project_button(proj_btn);
}

void TopBar::select_project_button(ProjectButton* proj_btn)
{
    for (Item* item : m_buttons_wrapper->items()) {
        ProjectButton* btn = static_cast<ProjectButton*>(item);
        btn->set_selected(btn == proj_btn);
    }
}

void TopBar::remove_project_button(ProjectButton* proj_btn)
{
    m_project_interactor->remove_project(proj_btn->project_id());
    proj_btn->set_visible(false);//m_buttons_wrapper->remove(proj_btn);

    size_t selected_project_id = m_project_interactor->selected_project_id();

    for (Item* btn : m_buttons_wrapper->items()) {
        ProjectButton* button = static_cast<ProjectButton*>(btn);
        button->set_selected(button->project_id() == selected_project_id);
    }
}

void TopBar::synchronize()
{
    std::set<size_t> project_ids;
    const auto& projects = m_project_interactor->workbench().projects();
    size_t selected_project_id = m_project_interactor->selected_project_id();

    // check if some of existent objects was deleted, then remove those buttons
    for (Item* item : m_buttons_wrapper->items()) {
        ProjectButton* proj_btn = static_cast<ProjectButton*>(item);
        auto it = projects.find(proj_btn->project_id());
        if (it == projects.end()) {
            remove_project_button(proj_btn);
        }
        project_ids.emplace(proj_btn->project_id());
    }

    // add missed projects
    for (const auto& [project_id, project] : projects) {
        if (project_ids.find(project_id) == project_ids.end())
            add_project_button(project_id, selected_project_id == project_id);
    }
}

} // namespace Slic3r::App