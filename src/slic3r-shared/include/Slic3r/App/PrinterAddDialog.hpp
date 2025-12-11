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
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;

class PrinterAddDialog : public Yoga::Dialog
{
public:
    PrinterAddDialog(Navigator& navigator);

protected:
    void on_tab_selected(int current_index) override;
    void close_action() override;

private:
    void create_add_logical_printer_page();
    void create_add_physical_printer_page();

private:
    Navigator& m_navigator;
    Yoga::StackLayout* m_stack_layout{nullptr};

    Yoga::ButtonGroup m_group_search;

    using PageListView = Yoga::ListView<
        PageEntryButton,
        PageEntry,
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>>;

    Biz::UnsharedPointer<Biz::ObservableList<PageEntry>> m_list_vendors;
    PageListView* m_page_list_view = nullptr;
};

} // namespace Slic3r::App
