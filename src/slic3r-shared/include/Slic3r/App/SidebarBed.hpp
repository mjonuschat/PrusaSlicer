///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/LogicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/App/MaterialSelectionDialog.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class Navigator;

namespace Yoga {
class Text;
class PrinterSettingsButton;
class MaterialSettingsButton;
} // namespace Yoga

class SidebarBed :
    public Yoga::Window,
    public Biz::IListSelectionChangedListener,
    public Biz::ISelectedBedInstancesChangedListener
{
public:
    explicit SidebarBed(Biz::ProjectInteractor& project_interactor, Navigator& navigator);

    void on_list_selection_changed(Domain::SelectionId new_selection) override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;

    LogicalPrinterSettingsDialog& logical_printer_settings_dialog();
    PhysicalPrinterSettingsDialog& physical_printer_settings_dialog();
    PrinterAddDialog& printer_add_dialog();
    MaterialSelectionDialog& material_selection_dialog();

private:
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;

    Yoga::Text* m_bed_name{nullptr};
    Yoga::PrinterSettingsButton* m_physical_printer_button{nullptr};
    Yoga::PrinterSettingsButton* m_logical_printer_button{nullptr};

    std::shared_ptr<Yoga::ButtonGroup> m_material_button_group;
    using MaterialListViewFactory = Yoga::ViewFactory<
        Yoga::MaterialSettingsButton,
        Biz::Preset::PresetItemObservableList,
        std::weak_ptr<Yoga::ButtonGroup>,
        Biz::ProjectInteractor&>;
    using MaterialListView = Yoga::ListView<
        Yoga::MaterialSettingsButton,
        Biz::Preset::PresetItemObservableList,
        MaterialListViewFactory>;

    MaterialListView* m_list_view{nullptr};
    LogicalPrinterSettingsDialog m_logical_printer_settings_dialog;
    PhysicalPrinterSettingsDialog m_physical_printer_settings_dialog;
    PrinterAddDialog m_printer_add_dialog;
    MaterialSelectionDialog m_material_selection_dialog;
};

} // namespace Slic3r::App
