///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PageEntryButton.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

namespace Slic3r::App::Yoga {
class StackLayout;
}

namespace Slic3r::App {

class PrinterAddDialog : public Yoga::Dialog
{
public:
    PrinterAddDialog();
    ~PrinterAddDialog();

protected:
    void on_tab_selected(int current_index) override;

private:
    void create_add_logical_printer_page();
    void create_add_physical_printer_page();

private:
    Yoga::StackLayout* m_stack_layout{nullptr};

    Yoga::ButtonGroup m_group_search;

    using PageListView = Yoga::ListView<
        PageEntryButton,
        PageEntry,
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>>;

    Biz::ObservableList<PageEntry> m_list_vendors;
    PageListView* m_page_list_view = nullptr;
};

} // namespace Slic3r::App
