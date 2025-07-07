///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PageEntryButton.hpp"

namespace Slic3r::App::Yoga {
class StackLayout;
class ScrollArea;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Yoga {

class AbstractSettingsDialog : public Dialog
{
public:
    explicit AbstractSettingsDialog(const std::string& tab);
    explicit AbstractSettingsDialog(const std::initializer_list<std::string>& tabs);

protected:
    struct RowItem
    {
        ItemPtr input;
        std::string label;
        std::string symbol;
    };

    void emplace_subcategory(
        Item* container,
        const std::string& name,
        const std::string& description,
        std::vector<RowItem>&& row_items
    );

    ScrollArea* emplace_stack_page();

protected:
    using PageListView = Yoga::ListView<
        PageEntryButton,
        PageEntry,
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>>;

    PageListView* m_page_list_view = nullptr;
    Yoga::StackLayout* m_pages_stack_layout = nullptr;
    Item* m_footer = nullptr;
};

} // namespace Slic3r::App::Yoga
