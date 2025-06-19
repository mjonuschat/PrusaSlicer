///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ItemEvents.hpp"

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

Event::Event(Item* item) : m_item(item) { ASSERT(item); }

Event::~Event() {}

void Event::affected(const ChangeList& change_list) {}

RemoveEvent::RemoveEvent(Item* item) : Event(item) {}

Event::ChangeList RemoveEvent::process()
{
    Item* parent = m_item->parent();
    ASSERT(parent);
    std::optional<size_t> index = parent->index_of(m_item);
    ASSERT(index.has_value());

    parent->remove(m_item);

    return {{Change::AffectedResult::Removed, parent, index.value()}};
}

MoveEvent::MoveEvent(Item* item, Item* new_parent, size_t new_index) : Event(item) {}

Event::ChangeList MoveEvent::process()
{
    Item* parent = m_item->parent();
    ASSERT(parent);
    std::optional<size_t> index = parent->index_of(m_item);
    ASSERT(index.has_value());

    m_new_parent->insert(parent->remove(m_item), m_new_index);

    return {
        {Change::AffectedResult::Removed, parent, index.value()},
        {Change::AffectedResult::Inserted, m_new_parent, m_new_index}
    };
}

void LoopEvents::insert_event(EventPtr event) {
    ASSERT(event);
    m_events.push_back(std::move(event));
}

void LoopEvents::process_events()
{
    while (!m_events.empty()) {
        EventPtr event(std::move(m_events.front()));
        m_events.pop_front();

        const Event::ChangeList change_list = event->process();

        for (const EventPtr& event : m_events) {
            event->affected(change_list);
        }
    }
}

void MoveEvent::affected(const ChangeList& change_list)
{
    for (const Change& change : change_list) {
        if (!m_item->parent() || change.parent != m_item->parent()) {
            return;
        }

        switch (change.affected_result) {
        case Change::AffectedResult::Inserted:
            if (m_new_index >= change.new_index) {
                m_new_index++;
            }
            break;
        case Change::AffectedResult::Removed:
            if (m_new_index < change.new_index) {
                m_new_index--;
            }
            break;
        }
    }
}

} // namespace Slic3r::App::Yoga
