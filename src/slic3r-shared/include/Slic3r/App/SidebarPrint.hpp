///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/PrintSettingsDialog.hpp"
#include "Slic3r/App/Yoga/ComboBoxListViewSelection.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/App/SidebarToolHeadRow.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class PrintSettingsDialog;

namespace Yoga {
class LayoutButton;
class InputTextField;
class ComboBox;
class ScrollArea;
} // namespace Yoga

class SidebarPrint : public Yoga::Window
{
public:
    explicit SidebarPrint(Biz::ProjectInteractor& project_interactor);

private:
    void add_separator();
    void add_row(Item* container, const std::string& label, std::unique_ptr<Yoga::Item> control);

    void create_favorite_params();
    void create_favorite_params_page(Item* container);

private:
    using ToolHeadListView = Yoga::ListView<
        SidebarToolHeadRow,
        Biz::Preset::PresetItemObservableList,
        Yoga::ViewFactory<SidebarToolHeadRow, Biz::Preset::PresetItemObservableList, std::weak_ptr<Yoga::ButtonGroup>, Biz::ProjectInteractor&>>;

    Biz::ProjectInteractor& m_project_interactor;

    Yoga::LayoutButton* m_settings_set_btn{nullptr};
    Yoga::ButtonGroup m_group_extruder;
    Yoga::InputTextField* m_input_text_perimeters{nullptr};
    Yoga::ComboBox* m_combo_density{nullptr};
    Yoga::ComboBox* m_combo_pattern{nullptr};
    Item* m_tool_container{nullptr};
    std::vector<Item*> m_tools;
    std::shared_ptr<Yoga::ButtonGroup> m_group_print_tools;
    Yoga::ScrollArea* m_content_area{nullptr};
    Yoga::ComboBox* m_combo_tools{nullptr};
    Yoga::Item* m_favorite_params_layout{nullptr};
    Yoga::ComboBoxListViewSelection<Biz::Preset::PresetItem>* m_combo_print{nullptr};
    ToolHeadListView* m_tool_head_list_view{nullptr};

    PrintSettingsDialog m_print_settings_dialog;
};

} // namespace Slic3r::App
