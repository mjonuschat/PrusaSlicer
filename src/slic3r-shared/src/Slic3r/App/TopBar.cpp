#include "Slic3r/App/TopBar.hpp"

#include "Slic3r/App/Yoga/ProjectButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/MenuItem.hpp"

#include "Slic3r/App/SearchBar.hpp"
#include "Slic3r/App/Platform/AbstractRenderModule.hpp"
#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"
#include "Slic3r/App/MenuBuilder.hpp"
#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/Platform/CommandName.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

using namespace Yoga;
using namespace Slic3r::Biz;

using CommandName = Platform::CommandName;

TopBar::TopBar(
    Biz::ProjectInteractor* project_interactor,
    Platform::AbstractRenderModule* render_module,
    ThumbnailStore& thumbnail_store,
    Navigator& navigator,
    App::Undo::Store* undo_store
) :
    Window("TopBar"),
    m_selected_project_changed_listener_scope(*project_interactor, *this),
    m_project_interactor(*project_interactor),
    m_render_module(render_module),
    m_navigator(navigator),
    m_menu_command_registrar(*render_module, *project_interactor, navigator, thumbnail_store),
    m_menu_manager(render_module->menu_manager()),
    m_undo_store(undo_store)
{
    m_menu_command_registrar.register_top_bar_menus();
    if (m_undo_store) {
        m_undo_store->add_listener<Undo::IStoreChangedListener>(this);
    }

    set_padding(0);
    set_rounding(0.f);
    set_gap(0.f);
    set_flex_shrink(0);

    Rectangle* left_wrapper = emplace_back<Rectangle>();
    left_wrapper->set_flex_shrink(0);
    left_wrapper->set_align_items(YGAlignCenter);

#ifndef USE_NATIVE_MENU
    add_menu_btns(left_wrapper);
    for (LayoutButton* btn : std::initializer_list<LayoutButton*>{m_save_btn, m_show_ui_btn}) {
        if (!btn)
            continue;
        btn->callbacks().hovered_changed = [this](bool hovered)
        {
            if (hovered) {
                if (m_file_menu->opened())
                    m_file_menu->close();
                if (m_main_menu->opened())
                    m_main_menu->close();
            }
        };
    }
#endif // !USE_NATIVE_MENU

    add_save_project_btn(left_wrapper);
    if (m_undo_store) {
        add_undo_btn(left_wrapper);
        add_redo_btn(left_wrapper);
    }
    add_show_ui_btn(left_wrapper);

    m_list_view = emplace_back<ProjectButtonListView>(m_project_interactor);
    m_list_view->set_window_flags(
        ImGuiWindowFlags_HorizontalScrollbar // compute horizontal scroll range
        | ImGuiWindowFlags_NoScrollbar // don't show any scrollbar
        | ImGuiWindowFlags_NoScrollWithMouse // don't let ImGui consume wheel vertically
    );
    m_list_view->set_remap_horizontal_scroll(true);
    m_list_view->set_source_list(&project_interactor->observable_project_list());

    Rectangle* project_actions_wrapper = emplace_back<Rectangle>();
    project_actions_wrapper->set_flex_grow(1.);

    // add_new_project_btn(project_actions_wrapper);
    // add_expander_btn(project_actions_wrapper);

    Rectangle* right_wrapper = emplace_back<Rectangle>();
    right_wrapper->set_flex_shrink(0);
    m_search_bar = right_wrapper->emplace_back<SearchBar>(m_project_interactor, m_navigator);

    for (LayoutButton* btn :
         std::initializer_list<LayoutButton*>{
             m_save_btn,
             m_undo_btn,
             m_undo_stack_btn,
             m_redo_btn,
             m_redo_stack_btn,
             m_show_ui_btn,
             m_main_menu_btn,
             m_file_menu_btn,
         })
    {
        if (!btn)
            continue;
        btn->set_background_color(IM_COL32_BLACK_TRANS);
        btn->set_tooltip_position(Position::Bottom);
        btn->set_height(btn == m_main_menu_btn ? 30.f : 24.f);
    }

    for (Rectangle* wrapper :
         std::initializer_list<Rectangle*>{left_wrapper, right_wrapper, project_actions_wrapper})
    {
        wrapper->set_fill(m_theme->color_imgui(Platform::Color::WindowBg));
        wrapper->set_rounding(0.f);
        wrapper->set_gap(15.f);
        wrapper->set_padding({15.f, 5.f});
    }

    if (m_project_interactor.selected_project_id() != Domain::INVALID_ID) {
        on_selected_project_changed(m_project_interactor.selected_project_id());
    }
}

TopBar::~TopBar()
{
    if (m_undo_store) {
        m_undo_store->remove_listener<Undo::IStoreChangedListener>(this);
    }
}

void TopBar::on_selected_project_changed(size_t index)
{
    for (size_t button_index = 0; button_index < m_list_view->list_item_count(); ++button_index) {
        ProjectButton* button = m_list_view->item_at(button_index);
        if (button->project_id() == index) {
            m_list_view->scroll_at_item(m_list_view->get_item(button_index));
            button->set_separator_visible(false);
            if (button_index > 0) {
                m_list_view->item_at(button_index - 1)->set_separator_visible(false);
            }
        } else {
            button->set_separator_visible(true);
        }
    }
}

static void append_snapshot(
    Yoga::Menu& menu,
    std::size_t snapshot_id,
    const std::string& name,
    bool selected,
    ProjectInteractor& project_interactor
)
{
    const Render::Icon icon{selected ? Render::Icon::StarSolid : Render::Icon::None};
    Yoga::MenuItem* menu_item{menu.append_item(name, icon, "", false)};
    menu_item->set_enabled(!selected);
    menu_item->callbacks().action = [snapshot_id, &project_interactor]()
    { project_interactor.undo_provider().select_snapshot(snapshot_id); };
}

void TopBar::on_undo_store_changed(
    Domain::SelectionId project_id,
    const std::vector<std::pair<std::size_t, std::string>>& snapshots,
    std::size_t selected_index
)
{
    if (m_project_interactor.selected_project_id() != project_id) {
        return;
    }

    const bool undo_enalbed{m_render_module->command(CommandName::Undo).enabled()};
    const bool redo_enalbed{m_render_module->command(CommandName::Redo).enabled()};
    m_undo_btn->set_enabled(undo_enalbed);
    m_undo_stack_btn->set_enabled(undo_enalbed);
    m_redo_btn->set_enabled(redo_enalbed);
    m_redo_stack_btn->set_enabled(redo_enalbed);

    m_undo_menu->clear();
    m_redo_menu->clear();

    for (std::size_t index{selected_index + 1}; index-- > 0;) {
        const auto& [id, name]{snapshots.at(index)};
        append_snapshot(*m_undo_menu, id, name, index == selected_index, m_project_interactor);
    }

    for (std::size_t index{selected_index}; index < snapshots.size(); ++index) {
        const auto& [id, name]{snapshots.at(index)};
        append_snapshot(*m_redo_menu, id, name, index == selected_index, m_project_interactor);
    }
}

void TopBar::focus_search()
{
    m_search_bar->focus_search();
}

void TopBar::add_save_project_btn(Item* parent)
{
    m_save_btn = parent->emplace_back<LayoutButton>("", Render::Icon::TobBarSave, _u8L("Save"));
    m_render_module->command_binding_manager().bind_tb_item(CommandName::SaveProject, m_save_btn);
}

static void toggle_menu_visibility(Menu* menu)
{
    if (!menu)
        return;
    menu->opened() ? menu->close() : menu->open();
}

void TopBar::add_undo_btn(Item* parent)
{
    auto item{parent->emplace_back<Item>()};
    m_undo_btn = item->emplace_back<LayoutButton>("", Render::Icon::TopBarUndo, _u8L("Undo"));
    m_undo_stack_btn = item->emplace_back<LayoutButton>("", Render::Icon::CaretDown, _u8L("Undo stack"));
    m_undo_stack_btn->set_enabled(false);

    m_undo_menu = m_undo_stack_btn->emplace_back<Yoga::Menu>("undo_menu", Yoga::Position::Bottom);

    m_undo_stack_btn->callbacks().action = [this]() { toggle_menu_visibility(m_undo_menu); };

    m_render_module->command_binding_manager().bind_tb_item(CommandName::Undo, m_undo_btn);
}

void TopBar::add_redo_btn(Item* parent)
{
    auto item{parent->emplace_back<Item>()};
    m_redo_btn = item->emplace_back<LayoutButton>("", Render::Icon::TopBarRedo, _u8L("Redo"));
    m_redo_stack_btn = item->emplace_back<LayoutButton>("", Render::Icon::CaretDown, _u8L("Redo stack"));
    m_redo_stack_btn->set_enabled(false);

    m_redo_menu = m_redo_stack_btn->emplace_back<Yoga::Menu>("redo_menu", Yoga::Position::Bottom);

    m_redo_stack_btn->callbacks().action = [this]() { toggle_menu_visibility(m_redo_menu); };

    m_render_module->command_binding_manager().bind_tb_item(CommandName::Redo, m_redo_btn);
}

void TopBar::add_show_ui_btn(Item* parent)
{
    m_show_ui_btn =
        parent->emplace_back<LayoutButton>("", Render::Icon::TobBarShowUI, _u8L("Hide sidebars"));
    m_show_ui_btn->set_checkable(true);

    m_show_ui_btn->callbacks().action = [this]()
    {
        m_show_ui_btn->set_tooltip(
            m_show_ui_btn->checked() ? _u8L("Show sidebars") : _u8L("Hide sidebars")
        );
        // Propagate sidebars visibility into active RenderModule
        m_render_module->set_sidebars_visible(!m_show_ui_btn->checked());

        // ysTODO: save hide value into app_config
    };
}

void TopBar::add_menu_btns(Item* parent)
{
    MenuBuilder menu_builder(
        m_render_module->menu_manager(),
        m_render_module->command_binding_manager()
    );

    if (App::MenuItem* main_menu_item =
            m_render_module->menu_manager().menu_item(MenuItemName::MainMenu))
    {
        m_main_menu_btn = parent->emplace_back<LayoutButton>(
            MenuBuilder::item_name_translated(main_menu_item->name()),
            MenuBuilder::item_icon(main_menu_item->name())
        );
        m_main_menu_btn->set_background_color(IM_COL32_BLACK_TRANS);
        m_main_menu_btn->callbacks().action = [this]() { toggle_menu_visibility(m_main_menu); };
        m_main_menu_btn->callbacks().hovered_changed = [this](bool hovered)
        {
            if (hovered && m_file_menu->opened())
                m_main_menu->open();
        };

        m_main_menu =
            m_main_menu_btn->emplace_back<Yoga::Menu>("main_menu", Yoga::Position::Bottom);
        m_main_menu->set_offset(0.f);

        menu_builder.add_menu_items(m_main_menu, main_menu_item);
    }

    if (App::MenuItem* file_menu_item =
            m_render_module->menu_manager().menu_item(MenuItemName::FileMenu))
    {
        m_file_menu_btn = parent->emplace_back<LayoutButton>(
            MenuBuilder::item_name_translated(file_menu_item->name()),
            MenuBuilder::item_icon(file_menu_item->name())
        );
        m_file_menu_btn->set_background_color(IM_COL32_BLACK_TRANS);
        m_file_menu_btn->callbacks().action = [this]() { toggle_menu_visibility(m_file_menu); };
        m_file_menu_btn->callbacks().hovered_changed = [this](bool hovered)
        {
            if (hovered && m_main_menu->opened())
                m_file_menu->open();
        };

        m_file_menu =
            m_file_menu_btn->emplace_back<Yoga::Menu>("file_menu", Yoga::Position::Bottom);
        m_file_menu->set_offset(0.f);
        menu_builder.add_menu_items(m_file_menu, file_menu_item);
    }
}

void TopBar::register_context_menus(
    Scene::GeometryDataFactory& data_factory,
    Scene::ISceneProvider* scene_provider
)
{
    m_menu_command_registrar.register_context_menus(data_factory, scene_provider);
}

} // namespace Slic3r::App
