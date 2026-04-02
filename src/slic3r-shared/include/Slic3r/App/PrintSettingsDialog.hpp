///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/ConfigDef.hpp>

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/ObservableListTransformer.hpp"
#include "Slic3r/Biz/PrintToolItem.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

#include "Slic3r/App/Config/PrintToolSubcategoryListView.hpp"
#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PageEntryButton.hpp"
#include "Slic3r/App/IConfigNavigable.hpp"
#include "Slic3r/App/PrintMetadataSettings.hpp"
#include "Slic3r/App/ToolLabel.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class PrintToolConfigObservableList;
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Scene {
class SceneInteractor;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::App::Yoga {
class StackLayout;
class Menu;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;

class PrintSettingsDialog :
    public Yoga::Dialog,
    public IConfigNavigable,
    public Biz::IObservableList<bool>,
    public Biz::ISelectedBedInstancesChangedListener,
    public Biz::Scene::ISceneBedInstanceChangedListener,
    public Biz::Preset::IPresetChangedListener
{
public:
    explicit PrintSettingsDialog(Biz::ProjectInteractor& project_interactor, Navigator& navigator);
    ~PrintSettingsDialog();

    const bool& at(size_t index) const override;
    size_t size() const override;

    void navigate_to_item(const Domain::ConfigItem* config_item) override;
    void clear_navigation() override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;

    void on_bed_instance_extruder_candidates_changed(
        Domain::SelectionId project_id,
        Domain::BedRef instance,
        const std::vector<unsigned int>& extruder_candidates
    ) override;

    void on_preset_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::PresetItemType type
    ) override;

    void on_config_container_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id
    ) override;

protected:
    using PageListView = Yoga::ListView<
        PageEntryButton,
        PageEntry,
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>>;

    using ToolPrintCategorizer =
        Biz::ObservableListSortFilter<Biz::PrintToolItem, Domain::ConfigItemDef::Category>;

    using ToolPrintMenuTransformer = Biz::ObservableListTransformer<Biz::PrintToolItem, PageEntry>;
    using ExtruderMenuTransformer =
        Biz::ObservableListTransformer<Biz::Preset::ToolConfigItemObservableList, PageEntry>;

    using ToolPrintCategoryFactory = Yoga::ViewFactory<
        PrintToolSubcategoryListView,
        Biz::PrintToolItem,
        Biz::PrintToolConfigBoxInteractor&,
        Biz::IConfigBoxSetter&,
        Biz::ProjectInteractor&>;
    using ToolPrintCategoryListView = Yoga::ListView<
        PrintToolSubcategoryListView,
        Biz::PrintToolItem,
        ToolPrintCategoryFactory,
        Yoga::StackLayout>;

    using PrintMetadataListView = Yoga::ListView<
        PrintMetadataSettings,
        Biz::Preset::ToolConfigItemObservableList,
        Yoga::ViewFactory<
            PrintMetadataSettings,
            Biz::Preset::ToolConfigItemObservableList,
            Biz::Preset::PresetInteractor&>,
        Yoga::StackLayout>;

    using ToolLabelFactory  = Yoga::ViewFactory<ToolLabel, bool, Biz::ProjectInteractor&>;
    using ToolLabelListView = Yoga::ListView<ToolLabel, bool, ToolLabelFactory>;

    void close_action() override;

    void select_page_entry(size_t index, bool category);

    void update_extruder_candidates();

    void update_extruder_size();

private:
    void update_extruder_candidates_internal();

    void on_about_to_close() override;

private:
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;

    Biz::ListenerScope<
        Biz::ISelectedBedInstancesChangedListener,
        Biz::Scene::SceneInteractor,
        PrintSettingsDialog>
        m_selected_bed_changed_scope;

    Biz::ListenerScope<
        Biz::Scene::ISceneBedInstanceChangedListener,
        Biz::Scene::SceneInteractor,
        PrintSettingsDialog>
        m_scene_bed_instance_changed_scope;

    Biz::ListenerScope<
        Biz::Preset::IPresetChangedListener,
        Biz::Preset::PresetInteractor,
        PrintSettingsDialog>
        m_preset_changed_listener_scope;

    Yoga::StackLayout* m_content_stack_layout{nullptr};
    ToolPrintCategoryListView* m_category_stack_list_view{nullptr};
    PrintMetadataListView* m_metadata_stack_list_view{nullptr};

    Yoga::Item* m_footer{nullptr};
    Yoga::LayoutButton* m_save_button{nullptr};
    Yoga::Menu* m_save_preset_menu{nullptr};
    PageListView* m_category_page_list_view{nullptr};
    PageListView* m_extruder_page_list_view{nullptr};

    Yoga::Text* m_bed_name{nullptr};
    ToolLabelListView* m_tool_label_list_view{nullptr};

    std::deque<bool> m_extruders;

    Biz::UnsharedPointer<ToolPrintCategorizer> m_tool_print_categorizer;
    Biz::UnsharedPointer<ToolPrintMenuTransformer> m_tool_print_transformer;
    Biz::UnsharedPointer<ExtruderMenuTransformer> m_extruder_menu_transformer;
};

} // namespace Slic3r::App
