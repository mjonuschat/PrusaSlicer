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
void emplace_row(
    Item* container, ItemPtr input, const std::string& label, const std::string& symbol = ""
)
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

AbstractSettingsDialog::AbstractSettingsDialog(const std::string& tab)
    : AbstractSettingsDialog({tab})
{}

AbstractSettingsDialog::AbstractSettingsDialog(const std::initializer_list<std::string>& tabs)
    : Dialog(tabs)
{
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(0);
    content()->set_flex_grow(1);
    // Only preserve 1px bottom padding
    content()->set_padding(Paddings(0, 0, 0, 1));

    Item* middle_row = content()->emplace_back<Item>();
    middle_row->set_orientation(Orientation::Horizontal);
    middle_row->set_flex_grow(1);

    // Create the ViewFactory explicitly:
    auto factory = Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>(
        [this](size_t index) {
            m_pages_stack_layout->set_current_index(index);
            for (size_t button_index = 0; button_index < m_page_list_view->item_count();
                 ++button_index) {
                PageEntryButton* button = dynamic_cast<PageEntryButton*>(
                    m_page_list_view->get_item(button_index)
                );
                ASSERT(button);
                button->set_checked(index == button_index);
            }
        }
    );
    m_page_list_view = middle_row->emplace_back<PageListView>(std::move(factory));
    m_page_list_view->set_orientation(Orientation::Vertical);
    m_page_list_view->set_min_size({125, 0});
    middle_row->emplace_back<Separator>(Orientation::Vertical);

    m_pages_stack_layout = middle_row->emplace_back<StackLayout>();
    m_pages_stack_layout->set_flex_grow(1);

    add_separator();

    m_footer = content()->emplace_back<Item>();
    m_footer->set_padding(10);
    m_footer->set_justify_content(YGJustifyFlexEnd);
    m_footer->set_self_align(YGAlignFlexEnd);
    m_footer->set_flex_shrink(0);
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
    ScrollArea* stacked_page = m_pages_stack_layout->emplace_back<ScrollArea>();
    stacked_page->set_orientation(Orientation::Vertical);
    stacked_page->set_flex_grow(1);
    stacked_page->set_min_size({0, 100});
    stacked_page->set_padding(20);

    return stacked_page;
}

} // namespace Slic3r::App::Yoga
