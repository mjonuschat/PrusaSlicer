///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/Yoga/ComboBoxListViewSelection.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/App/SidebarToolHeadRow.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class PrintSettingsDialog;
class PrintToolFavoritesItem;
class Navigator;

namespace Yoga {
class LayoutButton;
class InputTextField;
class ComboBox;
class ScrollArea;
} // namespace Yoga

class SidebarPrint : public Yoga::Window, public Biz::Preset::IPresetChangedListener
{
public:
    explicit SidebarPrint(Biz::ProjectInteractor& project_interactor, Navigator& navigator);

    PrintSettingsDialog& print_settings_dialog();

    void on_preset_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::PresetItemType type
    ) override;

    void on_config_container_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id
    ) override;

private:
    void add_separator();

    void create_favorite_params();

    void update_tools_visibility();

private:
    using ToolHeadListView = Yoga::ListView<
        SidebarToolHeadRow,
        Biz::Preset::PresetItemObservableList,
        Yoga::ViewFactory<
            SidebarToolHeadRow,
            Biz::Preset::PresetItemObservableList,
            Biz::ProjectInteractor&>>;

    Biz::ListenerScope<
        Biz::Preset::IPresetChangedListener,
        Biz::Preset::PresetInteractor,
        SidebarPrint>
        m_preset_changed_listener_scope;

    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;

    Yoga::LayoutButton* m_settings_set_btn{nullptr};
    Yoga::ButtonGroup m_group_extruder;
    Yoga::InputTextField* m_input_text_perimeters{nullptr};
    Yoga::ComboBox* m_combo_density{nullptr};
    Yoga::ComboBox* m_combo_pattern{nullptr};
    Item* m_tool_container{nullptr};
    std::vector<Item*> m_tools;
    Yoga::ScrollArea* m_content_area{nullptr};
    Yoga::ComboBox* m_combo_tools{nullptr};
    Yoga::Item* m_favorite_params_layout{nullptr};
    PrintToolFavoritesItem* m_favorite_params{nullptr};

    Yoga::ComboBoxListViewSelection<Biz::Preset::PresetItem>* m_combo_print{nullptr};
    ToolHeadListView* m_tool_head_list_view{nullptr};

    PrintSettingsDialog* m_print_settings_dialog{nullptr};

    int m_last_selected_index{-1};
};

} // namespace Slic3r::App
