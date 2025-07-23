///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"

#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace {
void emplace_row(Item* container, ItemPtr input, const std::string& label, const std::string& symbol = "")
{
    ASSERT(container);
    ASSERT(input);

    Item* row = container->emplace_back<Item>();

    row->set_flex_grow(1);
    row->set_flex_shrink(0);
    row->set_align_items(YGAlign::YGAlignCenter);

    Text* text = row->emplace_back<Text>(label);
    text->set_width(175);

    input->set_width(150);
    row->append(std::move(input));

    if (!symbol.empty()) {
        row->emplace_back<Text>(symbol);
    }
}
} // namespace

namespace Slic3r::App::Yoga {

AbstractSettingsDialog::AbstractSettingsDialog(const std::initializer_list<std::string>& tabs) :
    Dialog()
{
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(0);
    content()->set_flex_grow(1);
    // Only preserve 1px bottom padding
    content()->set_padding(Paddings(0, 0, 0, 1));

    m_stack_tabs = content()->emplace_back<StackLayout>();
    m_stack_tabs->set_orientation(Orientation::Vertical);
    m_stack_tabs->set_flex_grow(1);

    add_separator();

    m_footer = content()->emplace_back<Item>();
    m_footer->set_padding(10);
    m_footer->set_justify_content(YGJustifyFlexEnd);
    m_footer->set_self_align(YGAlignFlexEnd);
    m_footer->set_flex_shrink(0);

    for (const std::string& tab : tabs) {
        append_tab(tab);
    }
}

void AbstractSettingsDialog::emplace_subcategory(
    Item* container,
    const std::string& name,
    const std::string& description,
    std::vector<RowItem>&& row_items
)
{
    Item* subcategory = container->emplace_back<Item>();
    subcategory->set_orientation(Orientation::Vertical);
    subcategory->emplace_back<Text>(name, ImguiFontType::Bold);
    subcategory->set_flex_shrink(0);
    subcategory->set_margin(Margins(0, 0, 0, 10));

    Item* inputs = subcategory->emplace_back<Item>();
    inputs->set_orientation(Orientation::Vertical);
    inputs->set_margin(20);
    inputs->set_gap(5);

    for (RowItem& item : row_items) {
        emplace_row(inputs, std::move(item.input), item.label, item.symbol);
    }

    if (!description.empty()) {
        Text* desc = subcategory->emplace_back<Text>(description);
        desc->set_wrap(true);
    }
}

ScrollArea* AbstractSettingsDialog::emplace_stack_page()
{
    ScrollArea* stacked_page = m_current_tab->pages_stack_layout->emplace_back<ScrollArea>();
    stacked_page->set_orientation(Orientation::Vertical);
    stacked_page->set_flex_grow(1);
    stacked_page->set_min_size({0, 100});
    stacked_page->set_padding(20);

    return stacked_page;
}

void AbstractSettingsDialog::on_tab_selected(int current_index)
{
    m_current_tab = m_tabs.at(current_index).get();
    m_stack_tabs->set_current_index(current_index);
}

AbstractSettingsDialog::Tab* AbstractSettingsDialog::append_tab(const std::string& tab)
{
    Dialog::append_tab(tab);

    Item* tab_item = m_stack_tabs->emplace_back<Item>();
    tab_item->set_orientation(Orientation::Horizontal);

    // Create the ViewFactory explicitly:
    auto factory = Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>(
        [this](size_t index) {
        m_current_tab->pages_stack_layout->set_current_index(index);
        for (size_t button_index = 0; button_index < m_current_tab->page_list_view->item_count();
             ++button_index)
        {
            PageEntryButton* button = dynamic_cast<PageEntryButton*>(
                m_current_tab->page_list_view->get_item(button_index)
            );
            ASSERT(button);
            button->set_checked(index == button_index);
        }
    }
    );
    PageListView* page_list_view = tab_item->emplace_back<PageListView>(std::move(factory));
    page_list_view->set_orientation(Orientation::Vertical);
    page_list_view->set_min_size({125, 0});

    tab_item->emplace_back<Separator>(Orientation::Vertical);

    StackLayout* pages_stack_layout = tab_item->emplace_back<StackLayout>();
    pages_stack_layout->set_flex_grow(1);

    m_tabs.emplace_back(std::make_unique<Tab>(page_list_view, pages_stack_layout));

    if (!m_current_tab) {
        m_current_tab = m_tabs.back().get();
    }

    return m_tabs.back().get();
}

} // namespace Slic3r::App::Yoga
