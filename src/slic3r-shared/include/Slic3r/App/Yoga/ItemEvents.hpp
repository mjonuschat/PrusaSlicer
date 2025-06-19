///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <memory>
#include <list>

namespace Slic3r::App::Yoga {

class Item;

class Event
{
public:
    struct Change
    {
        enum class AffectedResult
        {
            Removed,
            Inserted,
        };
        AffectedResult affected_result{AffectedResult::Removed};
        Item* parent{nullptr};
        size_t new_index{0};
    };
    using ChangeList = std::list<Change>;

    Event(Item* item);
    virtual ~Event();

    virtual ChangeList process() = 0;

    virtual void affected(const ChangeList& change_list);

protected:
    Item* m_item = nullptr;
};
using EventPtr = std::unique_ptr<Event>;

class RemoveEvent : public Event
{
public:
    RemoveEvent(Item* item);

    ChangeList process() override;
};

class MoveEvent : public Event
{
public:
    MoveEvent(Item* item, Item* new_parent, size_t new_index);

    ChangeList process() override;

    void affected(const ChangeList& change_list) override;

private:
    Item* m_new_parent = nullptr;
    size_t m_new_index = 0;
};

/**
 * @brief The LoopEvents class - Stores and processes Events
 * Each event can produce ChangeList which then can affect other
 * events in loop
 */
class LoopEvents
{
public:
    void insert_event(EventPtr event);

    void process_events();

private:
    std::list<EventPtr> m_events;
};

} // namespace Slic3r::App::Yoga
