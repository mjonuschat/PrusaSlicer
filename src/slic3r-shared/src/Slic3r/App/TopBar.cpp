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
#include "Slic3r/App/ThumbnailStore.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using namespace Yoga;

TopBar::TopBar(
    Biz::ProjectInteractor* project_interactor,
    Platform::AbstractRenderModule* render_module,
    ThumbnailStore& thumbnail_store
) :
    Window("top_bar"),
    m_project_interactor(project_interactor),
    m_render_module(render_module),
    m_thumbnail_store(thumbnail_store)
{
    Paddings paddings = padding();

    set_padding(0);
    set_alpha(0.f);
    set_rounding(0.f);
    set_gap(0.f);
    set_flex_shrink(0);

    Rectangle* left_wrapper = emplace_back<Rectangle>();

    add_load_project_btn(left_wrapper);
    add_save_project_btn(left_wrapper);
    add_show_ui_btn(left_wrapper);

    m_list_view = emplace_back<ProjectButtonListView>(*m_project_interactor);
    m_list_view->set_source_list(&project_interactor->observable_project_list());

    Rectangle* project_actions_wrapper = emplace_back<Rectangle>();
    project_actions_wrapper->set_flex_grow(1.);

    add_new_project_btn(project_actions_wrapper);
    add_expander_btn(project_actions_wrapper);

    Rectangle* right_wrapper = emplace_back<Rectangle>();
    m_search                 = right_wrapper->emplace_back<Rectangle>();
    m_search->set_min_size({200.f, YGUndefined});
    m_search->set_rounding(0.f);

    Text* shortcut = right_wrapper->emplace_back<Text>("Ctrl+F");
    shortcut->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    shortcut->set_self_align(YGAlignCenter);

    for (LayoutButton* btn : std::initializer_list<LayoutButton*>{
             m_load_btn,
             m_save_btn,
             m_show_ui_btn,
             m_new_btn,
         })
    {
        btn->set_background_color(IM_COL32_BLACK_TRANS);
        btn->set_tooltip_position(Position::Bottom);
    }

    for (Rectangle* wrapper : std::initializer_list<Rectangle*>{
             left_wrapper,
             right_wrapper,
             project_actions_wrapper,
         })
    {
        wrapper->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);
        wrapper->set_rounding(0.f);
        wrapper->set_gap(15.f);
        wrapper->set_padding(paddings);
    }
}

void TopBar::add_load_project_btn(Item* parent)
{
    m_load_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarLoad, _u8L("Load"));
    m_load_btn->callbacks().action = [this]() {
        IDialogManager::FileCallback callback =
            [this](bool success, const std::vector<boost::filesystem::path>& file_paths) {
            if (success) {
                m_project_interactor->load_project(file_paths.front());
            }
        };

        auto& dlg_manager = DialogManagerProvider::instance().get();
        dlg_manager
            .show_file_dialog(FileDialogType::Open, _u8L("Open Project"), "", "", "*.3mf", callback);
    };
}

void TopBar::add_save_project_btn(Item* parent)
{
    m_save_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarSave, _u8L("Save"));
    m_save_btn->callbacks().action = [this]() {
        auto& dlg_manager = DialogManagerProvider::instance().get();
        dlg_manager.show_yesno_dialog(
            "DEVELOPER WARNING",
            "EXPORT TO 3MF IS NOT FINALIZED YET.\n\nThe exported project MUST NOT be shared publicly, "
            "it will not be compatible with both old PrusaSlicer and the finalized 3.0.0.\n\n"
            "Do you really want to export it?",
            [this](bool answer) {
            if (!answer)
                return;
            Domain::SelectionId selected_project_id = m_project_interactor->selected_project_id();
            const std::string& project_name         = m_project_interactor->get_project_name(
                selected_project_id
            );
            Store3mfParam params{
                .thumbnail = m_thumbnail_store.projects.selected().thumbnail_3mf.get()
            };
            if (true || project_name.empty())
            { // The 'true' is here for the development phase - effectively it always "Saves as".
                // Saving a new project - show file save dialog.
                IDialogManager::FileCallback callback =
                    [this,
                     &params](bool success, const std::vector<boost::filesystem::path>& file_paths) {
                    if (success) {
                        std::string file_path = file_paths.front().string();
                        // file path could have locale dependent characters, do not use tolower
                        if (!file_path.ends_with(".3mf") && !file_path.ends_with(".3MF")) {
                            file_path.append(".3mf");
                        }

                        m_project_interactor->save_project(file_path, params);
                    }
                };
                auto& dlg_manager = DialogManagerProvider::instance().get();
                dlg_manager.show_file_dialog(
                    FileDialogType::Save,
                    _u8L("Save Project"),
                    "",
                    "",
                    "*.3mf",
                    callback
                );
            } else {
                // Saving an existing project - just save.
                m_project_interactor->save_project(project_name, params);
            }
        }
        );
    };
}

void TopBar::add_show_ui_btn(Item* parent)
{
    m_show_ui_btn = parent->emplace_back<LayoutButton>(
        "",
        Render::Icon::TobBarShowUI,
        _u8L("Hide sidebars")
    );
    m_show_ui_btn->set_checkable(true);

    m_show_ui_btn->callbacks().action = [this]() {
        m_show_ui_btn->set_tooltip(
            m_show_ui_btn->checked() ? _u8L("Show sidebars") : _u8L("Hide sidebars")
        );
        // Propagate sidebars visibility into active RenderModule
        m_render_module->set_sidebars_visible(!m_show_ui_btn->checked());

        // ysTODO: save hide value into app_config
    };
}

void TopBar::add_new_project_btn(Item* parent)
{
    m_new_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarPlus, "Add new project");
    m_new_btn->set_background_color(IM_COL32_BLACK_TRANS);
    m_new_btn->callbacks().action = [this]() {
        m_project_interactor->new_project();
    };
}

void TopBar::add_expander_btn(Item* parent)
{
    m_expand_btn = parent->emplace_back<LayoutButton>("", Render::Icon::OpenArrow);
    m_expand_btn->set_background_color(IM_COL32_BLACK_TRANS);
    m_expand_btn->set_visible(false);
}

} // namespace Slic3r::App
