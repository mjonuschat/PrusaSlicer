///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/App/LogicalPrinter.hpp"
#include "Slic3r/App/LogicalPrinterSettingsButton.hpp"

namespace Slic3r::App {
class PrinterAddDialog;

class PrinterSettingsDialog : public Yoga::Dialog
{
public:
    PrinterSettingsDialog(PrinterAddDialog* printer_add_dialog);

private:
    void create_page_list();
    void create_page_settings();

private:
    using PrinterListView = Yoga::ListView<
        LogicalPrinterSettingsButton,
        LogicalPrinter,
        Yoga::ViewFactory<
            LogicalPrinterSettingsButton,
            LogicalPrinter,
            LogicalPrinterSettingsButton::FnIndexClicked>>;

    Biz::ObservableList<LogicalPrinter> m_list_logical_printers;
    PrinterListView* m_printer_list_view{nullptr};
    Yoga::StackLayout* m_stack_layout{nullptr};
    Yoga::Item* m_page_list{nullptr};
    Yoga::Item* m_page_settings{nullptr};
    Yoga::ButtonGroup m_group_keywords;
    PrinterAdvancedSettingsDialog m_advanced_dialog;
    PrinterAddDialog* m_printer_add_dialog{nullptr};
};

} // namespace Slic3r::App
