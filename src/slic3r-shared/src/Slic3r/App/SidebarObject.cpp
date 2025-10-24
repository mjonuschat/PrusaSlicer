///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/SidebarObject.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App {

SidebarObject::SidebarObject(Biz::ProjectInteractor& project_interactor) :
    Window("SidebarObject"),
    m_project_interactor(project_interactor),
    m_scene_selection_changed_listener_scope(project_interactor.scene_interactor(), *this),
    m_override_settings_dialog(m_project_interactor)
{
    set_orientation(Orientation::Vertical);
    set_min_size({240, 60});
    set_flex_grow(1);
    set_padding(0);

    ScrollArea* scroll_area = emplace_back<ScrollArea>();
    scroll_area->set_orientation(Orientation::Vertical);
    scroll_area->set_gap(5);
    scroll_area->set_padding(Paddings(5, 5, 5, 5));

    m_text_object_name = scroll_area->emplace_back<Text>("Unkown");
    m_text_object_name->set_font_type(Render::ImguiFontType::Bold);
    m_text_object_name->set_flex_shrink(0);

    m_config_item_filter = std::make_shared<ConfigItemFilter>();
    m_config_item_filter->set_filter_fn([this](const Biz::OverrideItem& item) -> bool {
        return !item.is_override()
            && m_project_interactor.preset_interactor().tool_cbi_list().size() > 1;
    });

    m_config_item_list_view = scroll_area->emplace_back<ConfigItemListView>(
        m_project_interactor.preset_interactor()
    );
    m_config_item_list_view->set_orientation(Orientation::Vertical);
    m_config_item_list_view->set_gap(5);
    m_config_item_list_view->set_flex_shrink(0);
    m_config_item_list_view->set_source_list(m_config_item_filter.get());

    std::weak_ptr<Biz::ObjectSettingsObservableList>
        object_settings_observable_list = m_project_interactor.preset_interactor()
                                              .object_settings_interactor()
                                              .object_observable_list();
    m_config_item_filter->set_source_model(object_settings_observable_list);

    LayoutButton* add_settings_button = scroll_area->emplace_back<LayoutButton>(
        _u8L("Add object settings")
    );
    add_settings_button->set_self_align(YGAlignCenter);
    add_settings_button->callbacks().action = [this] {
        if (m_override_settings_dialog.opened()) {
            m_override_settings_dialog.close();
        } else {
            m_override_settings_dialog.open();
        }
    };
    add_settings_button->set_flex_shrink(0);

    m_override_group_filter = std::make_shared<ObservableOverrideCategorizer>();
    m_override_group_filter->set_allow_disabled(false);

    m_override_group_list_view = scroll_area->emplace_back<OverrideGroupListView>(m_project_interactor);
    m_override_group_list_view->set_orientation(Orientation::Vertical);
    m_override_group_list_view->set_gap(5);
    m_override_group_list_view->set_flex_shrink(0);
    m_override_group_list_view->set_source_list(m_override_group_filter.get());

    m_override_group_filter->set_source_model(object_settings_observable_list);

    m_override_settings_dialog.attach_to_item(this, Position::Left);
    m_override_settings_dialog.callbacks().opened = [add_settings_button] {
        add_settings_button->set_checked(true);
    };
    m_override_settings_dialog.callbacks().closed = [add_settings_button] {
        add_settings_button->set_checked(false);
    };
}

void SidebarObject::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    if (m_project_interactor.selected_project_id() != project_id) {
        return;
    }

    m_selection = selection;
    update_object_name();
}

void SidebarObject::visible_updated_internal()
{
    if (!is_visible()) {
        m_override_settings_dialog.close();
    }
}

void SidebarObject::update_object_name()
{
    if (m_selection.empty()) {
        return;
    }

    std::string text;
    if (m_selection.elements.size() > 1) {
        text = fmt::format("{} Selected", m_selection.elements.size());
    } else if (m_selection.mode == Biz::Scene::SelectionMode::Instance) {
        Domain::ModelObject* object = m_project_interactor.selected_project().find_object_by_id(
            m_selection.elements.front().object_id
        );
        ASSERT(object);
        text = object->name;
    } else if (m_selection.mode == Biz::Scene::SelectionMode::Volume) {
        Domain::ModelVolume* volume = m_project_interactor.selected_project().find_volume_by_id(
            m_selection.elements.front().object_id,
            m_selection.elements.front().volume_id
        );
        ASSERT(volume);
        text = volume->name;
    }

    m_text_object_name->set_text(text);
}

} // namespace Slic3r::App
