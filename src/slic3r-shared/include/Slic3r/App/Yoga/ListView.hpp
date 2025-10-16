///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

/**
 * @brief The ViewFactory class is factory for View, the only reason behind it is to provide
 * "global" like attributes that are same for every View/Data, most probably you will supply here
 * pointer/references to interactors
 */
template <class View, class Data, typename... Args>
class ViewFactory
{
public:
    ViewFactory(Args... args) : m_args(args...) {}

    std::unique_ptr<View> create(size_t index, const Data& data)
    {
        return std::apply(
            [&](auto&&... args) { return std::make_unique<View>(index, data, args...); },
            m_args
        );
    }

private:
    std::tuple<Args...> m_args;
};

/**
 * @brief The ListView class is a template rich class which you will instantiate whenever you want
 * to dynamically show/create Views based on Data supplied by ObservableList.
 * You probably don't want to derive from this class but simply use it, all use cases should be
 * handled already. If you need to supply some global attributes to your Views, simply define a
 * ViewFactory and pass it in the constructor.
 */
template <
    class View,
    class Data,
    class ViewFactoryT = ViewFactory<View, Data>,
    class BaseItem     = Item>
class ListView : public Biz::IListObserver<Data>, public BaseItem
{
    using Items = std::vector<View*>;

public:
    ListView(ViewFactoryT factory = {}) : m_factory(factory) {}

    ~ListView()
    {
        if (m_source_list.is_valid()) {
            m_source_list->template remove_listener<Biz::IListObserver<Data>>(this);
        }
    }

    size_t list_item_count() const
    {
        return m_items.size();
    }

    View* item_at(size_t index) const
    {
        return m_items.at(index);
    }

    void on_inserted(const Data& data, size_t index) override
    {
        ASSERT(index <= m_items.size());

        Passthrough<View> item_view(m_factory.create(index, data));
        this->insert(item_view.release(), index);
        m_items.emplace(m_items.cbegin() + index, item_view.get());

        update_indexes(++index);
    }

    void on_will_be_removed(const Biz::IndexRange& index_range) override
    {
        ASSERT(index_range.to <= m_items.size());

        for (size_t i = index_range.from; i <= index_range.to; ++i) {
            m_items.at(i)->on_will_be_removed();
        }
    }

    /**
     * @warning Must be called only after the Data object is already removed.
     */
    void on_removed(const Biz::IndexRange& index_range) override
    {
        ASSERT(index_range.to <= m_items.size());

        for (size_t i = index_range.from; i <= index_range.to; ++i) {
            this->remove_later(m_items[i]);
        }
        m_items.erase(m_items.cbegin() + index_range.from, m_items.cbegin() + index_range.to + 1);
        update_indexes(index_range.from);
    }

    void on_updated(const Biz::IndexRange& index_range) override
    {
        ASSERT(index_range.to <= m_items.size());
        size_t index = 0;
        for (View* view : std::as_const(m_items)) {
            view->set_state(m_source_list->at(index++));
        }
    }

    void on_will_be_reset() override
    {
        for (View* view : std::as_const(m_items)) {
            view->on_will_be_removed();
        }
    }

    void on_reset() override
    {
        if (!m_items.empty()) {
            on_removed({0, m_items.size() - 1});
        }

        if (m_source_list.is_valid()) {
            for (size_t index = 0; index < m_source_list->size(); ++index) {
                on_inserted(m_source_list->at(index), index);
            }
        }
    }

    void on_moved(size_t from, size_t to) override
    {
        ASSERT(from < m_items.size() && to < m_items.size());
        if (from == to) {
            return;
        }

        update_indexes(from);
    }

    void set_source_list(Biz::IObservableList<Data>* source_list)
    {
        set_source_list(Biz::WeakerPointer<Biz::IObservableList<Data>>{source_list});
    }

    template <
        typename Derived,
        typename = std::enable_if_t<std::is_base_of_v<Biz::IObservableList<Data>, Derived>>>
    void set_source_list(const std::weak_ptr<Derived>& source_list)
    {
        set_source_list(
            Biz::WeakerPointer<Biz::IObservableList<Data>>{
                std::static_pointer_cast<Biz::IObservableList<Data>>(source_list.lock())
            }
        );
    }

    void set_source_list(const Biz::WeakerPointer<Biz::IObservableList<Data>>& source_list)
    {
        if (m_source_list != source_list) {
            if (m_source_list.is_valid()) {
                m_source_list->template remove_listener<Biz::IListObserver<Data>>(this);
            }

            m_source_list = source_list;
            if (m_source_list.get()) {
                m_source_list->template add_listener<Biz::IListObserver<Data>>(this);
            }

            on_reset();
        }
    }

private:
    void update_indexes(size_t from)
    {
        // update indexes for remaining objects at index_range.from+
        for (size_t new_index = from; new_index < m_items.size(); ++new_index) {
            m_items.at(new_index)->set_state_index(new_index, m_source_list->at(new_index));
        }
    }

private:
    Biz::WeakerPointer<Biz::IObservableList<Data>> m_source_list;
    ViewFactoryT m_factory;

    Items m_items;
};

} // namespace Slic3r::App::Yoga
