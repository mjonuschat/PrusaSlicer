///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/MaterialSelectionRow.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class InputText;
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;
class MaterialSettingsDialog;

class MaterialSelectionDialog :
    public Yoga::Dialog,
    public Biz::IListSelectionChangedListener,
    public Biz::IListObserver<Biz::Preset::PresetItemObservableList>
{
public:
    struct Callbacks
    {
        std::function<void(size_t tab_index)> advanced_settings_tab_opened;
    };

    MaterialSelectionDialog(Biz::ProjectInteractor& project_interactor, Navigator& navigator);
    ~MaterialSelectionDialog();

    Callbacks& material_selection_callbacks();

    void set_material_index(size_t material_index);

    void on_list_selection_changed(Domain::SelectionId new_selection) override;

    void on_will_be_reset() override;
    void on_reset() override;

    MaterialSettingsDialog& material_settings_dialog();

protected:
    void close_action() override;

    void update_preset_list();

private:
    using SelectionRowListViewFactory = Yoga::ViewFactory<
        MaterialSelectionRow,
        Biz::Preset::PresetItem,
        size_t&,
        Biz::Preset::PresetInteractor&>;
    using SelectionRowListView =
        Yoga::ListView<MaterialSelectionRow, Biz::Preset::PresetItem, SelectionRowListViewFactory>;

    Callbacks m_callbacks;

    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
    Biz::Preset::PresetItemCompoundObservableList& m_material_presets;
    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Biz::Preset::PresetItem>> m_material_filter;

    size_t m_material_index                              = Domain::INVALID_ID;
    Yoga::InputText* m_input_text_search                 = nullptr;
    SelectionRowListView* m_selection_row_list_view      = nullptr;
    Yoga::LayoutButton* m_advanced_button                = nullptr;
    Biz::Preset::PresetItemObservableList* m_preset_list = nullptr;
    MaterialSettingsDialog* m_material_settings_dialog   = nullptr;
};

} // namespace Slic3r::App
