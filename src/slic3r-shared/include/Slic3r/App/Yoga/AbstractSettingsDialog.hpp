///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PageEntryButton.hpp"

namespace Slic3r::Domain {
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::App::Yoga {
class StackLayout;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Yoga {

class AbstractSettingsDialog : public Dialog
{
public:
    explicit AbstractSettingsDialog(
        const std::initializer_list<std::string>& tabs,
        const std::string& name = {}
    );
    ~AbstractSettingsDialog();

protected:
    using PageListView = Yoga::ListView<
        PageEntryButton,
        PageEntry,
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>>;

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

    struct Tab
    {
        PageListView* page_list_view          = nullptr;
        Yoga::StackLayout* pages_stack_layout = nullptr;
    };

    void on_tab_selected(int current_index) override;

    Tab* append_tab(const std::string& tab);
    virtual void remove_tab(size_t index);

protected:
    using TabPtr = std::unique_ptr<Tab>;
    using Tabs   = std::vector<TabPtr>;
    Tabs m_tabs;
    Item* m_footer                  = nullptr;
    Tab* m_current_tab              = nullptr;
    Yoga::StackLayout* m_stack_tabs = nullptr;
    bool m_remove_in_progress       = false;
};

} // namespace Slic3r::App::Yoga
