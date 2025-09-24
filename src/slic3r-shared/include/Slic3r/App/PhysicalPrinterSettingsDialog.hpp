///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsButton.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App::Yoga {
class StackLayout;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;

class PrinterAddDialog;

class PhysicalPrinterSettingsDialog : public Yoga::Dialog
{
public:
    PhysicalPrinterSettingsDialog(PrinterAddDialog* printer_add_dialog, Navigator& navigator);
    ~PhysicalPrinterSettingsDialog();

protected:
    void close_action() override;

private:
    void create_page_list();
    void create_page_settings();

private:
    using PrinterListView = Yoga::ListView<
        PhysicalPrinterSettingsButton,
        PhysicalPrinter,
        Yoga::ViewFactory<PhysicalPrinterSettingsButton, PhysicalPrinter, PhysicalPrinterSettingsButton::FnIndexClicked>>;

    PrinterAddDialog* m_printer_add_dialog{nullptr};
    Navigator& m_navigator;

    Biz::ObservableList<PhysicalPrinter> m_list_physical_printers;
    PrinterListView* m_printer_list_view{nullptr};
    Yoga::StackLayout* m_stack_layout{nullptr};
    Yoga::Item* m_page_list{nullptr};
    Yoga::Item* m_page_settings{nullptr};
    Yoga::ButtonGroup m_group_keywords;
};

} // namespace Slic3r::App
