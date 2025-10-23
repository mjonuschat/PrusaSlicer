///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/ObjectConfigItem.hpp"
#include "Slic3r/App/OverrideOptionGroup.hpp"
#include "Slic3r/App/OverrideSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableOverrideCategorizer.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Text;
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class SidebarObject : public Yoga::Window, public Biz::Scene::ISceneSelectionChangedListener
{
public:
    explicit SidebarObject(Biz::ProjectInteractor& project_interactor);

    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    ) override;

protected:
    void visible_updated_internal() override;

private:
    void update_object_name();
    void update_enable_modifiers();

private:
    using ConfigItemListViewFactory = Yoga::ViewFactory<
        ObjectConfigItem,
        Biz::OverrideItem,
        Biz::Preset::PresetInteractor&>;
    using ConfigItemListView = Yoga::ListView<ObjectConfigItem, Biz::OverrideItem, ConfigItemListViewFactory>;

    using OverrideGroupListViewFactory = Yoga::ViewFactory<
        OverrideOptionGroup,
        Biz::OverrideItem,
        Biz::ProjectInteractor&>;
    using OverrideGroupListView = Yoga::ListView<OverrideOptionGroup, Biz::OverrideItem, OverrideGroupListViewFactory>;

    using ConfigItemFilter = Biz::ObservableListSortFilter<Biz::OverrideItem>;

    Biz::ProjectInteractor& m_project_interactor;

    Biz::ListenerScope<Biz::Scene::ISceneSelectionChangedListener, Biz::Scene::SceneInteractor, SidebarObject>
        m_scene_selection_changed_listener_scope;

    ConfigItemListView* m_config_item_list_view{nullptr};
    OverrideGroupListView* m_override_group_list_view{nullptr};

    Biz::UnsharedPointer<ConfigItemFilter> m_config_item_filter;
    Biz::UnsharedPointer<ObservableOverrideCategorizer> m_override_group_filter;

    Yoga::Text* m_text_object_name{nullptr};
    Yoga::LayoutButton* m_add_settings_button{nullptr};
    Biz::Scene::ObjectSelection m_selection;

    OverrideSettingsDialog m_override_settings_dialog;
};

} // namespace Slic3r::App
