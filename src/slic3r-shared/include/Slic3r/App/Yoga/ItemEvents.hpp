///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

#include <memory>
#include <list>

namespace Slic3r::App::Yoga {

class RootItem;

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

    virtual bool is_valid() const;

protected:
    Item* m_item = nullptr;
    ItemHeartBeat m_item_heartbeat;
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

    bool is_valid() const override;

private:
    Item* m_new_parent = nullptr;
    ItemHeartBeat m_new_parent_heartbeat;
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
    explicit LoopEvents(RootItem& root_item);

    void insert_event(EventPtr event);

    /**
     * @return number of processed events. if > 0, something in the tree did change
     */
    unsigned int process_events();

private:
    RootItem& m_root_item;
    std::list<EventPtr> m_events;
};

} // namespace Slic3r::App::Yoga
