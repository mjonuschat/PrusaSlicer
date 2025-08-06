///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/StackLayout.hpp"

namespace Slic3r::App::Yoga {

void StackLayout::insert(ItemPtr child, size_t index)
{
    if (!m_children.empty()) {
        child->set_visible(false);
    }

    Item::insert(std::move(child), index);
}

ItemPtr StackLayout::remove(Item* child)
{
    std::optional<size_t> index = index_of(child);
    ASSERT(index.has_value());

    ItemPtr item = Item::remove(child);

    if (m_current_index >= index.value()) {
        m_current_index--;
    }
    m_current_index = std::clamp(m_current_index, size_t{0}, item_count());

    return item;
}

size_t StackLayout::current_index() const { return m_current_index; }

void StackLayout::set_current_index(size_t current_index)
{
    if (m_current_index == current_index) {
        return;
    }

    ASSERT(current_index < item_count());

    m_current_index = current_index;

    for (size_t index = 0; index < item_count(); ++index) {
        get_item(index)->set_visible(m_current_index == index);
    }
}

} // namespace Slic3r::App::Yoga
