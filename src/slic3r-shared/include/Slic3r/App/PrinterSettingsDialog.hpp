///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ComboBoxListViewSelection.hpp"
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"
#include "Slic3r/App/LogicalPrinterSettingsButton.hpp"
#include "Slic3r/App/PrinterNozzleRow.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {
class PrinterAddDialog;

class PrinterSettingsDialog : public Yoga::Dialog
{
public:
    PrinterSettingsDialog(Biz::ProjectInteractor& project_interactor, PrinterAddDialog* printer_add_dialog);

private:
    void create_page_list();
    void create_page_settings();

private:
    void on_about_to_show() override;

private:
    using PrinterListView = Yoga::ListView<
        LogicalPrinterSettingsButton,
        Biz::Preset::PresetItem,
        Yoga::ViewFactory<LogicalPrinterSettingsButton, Biz::Preset::PresetItem, LogicalPrinterSettingsButton::FnIndexClicked>>;

    using NozzleListView = Yoga::ListView<
        PrinterNozzleRow,
        Biz::Preset::ToolConfigItemObservableList,
        Yoga::ViewFactory<PrinterNozzleRow, Biz::Preset::ToolConfigItemObservableList, Biz::Preset::PresetInteractor&>>;

    Biz::ProjectInteractor& m_project_interactor;
    PrinterListView* m_printer_list_view{nullptr};
    Yoga::StackLayout* m_stack_layout{nullptr};
    Yoga::Item* m_page_list{nullptr};
    Yoga::Item* m_page_settings{nullptr};
    Yoga::ButtonGroup m_group_keywords;
    Yoga::ComboBoxListViewSelection<Domain::Preset::HwSheetConfigDef>* m_combo_sheets;
    NozzleListView* m_nozzle_list_view{nullptr};
    PrinterAdvancedSettingsDialog m_advanced_dialog;
    PrinterAddDialog* m_printer_add_dialog{nullptr};
};

} // namespace Slic3r::App
