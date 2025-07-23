///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/IListObserver.hpp"

#include <unordered_set>

namespace Slic3r::Biz {

/**
 * @brief The ObservableListSortFilter class
 * @note it is advised that user first set all filter/sort/group function and then calls set_source_model,
 * otherwise invalidation will be done multiple times
 */
template <class Data, class GroupKey = std::string>
class ObservableListSortFilter : public Biz::IListObserver<Data>, public Biz::IObservableList<Data>
{
public:
    using FilterFn = std::function<bool(const Data& item)>;
    using SortFn   = std::function<int(const Data& lhs, const Data& rhs)>;
    using GroupByFn = std::function<bool(const Data& item, std::unordered_set<GroupKey>& seen_keys)>;

    void set_filter_fn(const FilterFn& filter_fn)
    {
        m_filter_fn = filter_fn;

        invalidate();
    }

    void set_sort_fn(const SortFn& sort_fn)
    {
        m_sort_fn = sort_fn;

        invalidate();
    }

    void set_group_by_fn(const GroupByFn& group_by)
    {
        m_group_by_fn = group_by;

        invalidate();
    }

    const Data& at(size_t index) const override
    {
        return m_source_model->at(m_mapped_indexes.at(index));
    }

    size_t size() const override
    {
        return m_mapped_indexes.size();
    }

    void on_inserted(const Data& data, size_t index) override
    {
        invalidate();
    }

    void on_removed(const Biz::IndexRange& index_range) override
    {
        invalidate();
    }

    void on_updated(const Biz::IndexRange& index_range) override
    {
        if (m_mapped_indexes.size()) {
            this->template invoke_listeners<IListObserver<Data>>([&](auto* l) {
                l->on_updated({0, m_mapped_indexes.size()});
            });
        }
    }

    void on_reset() override
    {
        invalidate();
    }

    void on_moved(size_t from, size_t to) override
    {
        invalidate();
    }

    IObservableList<Data>* source_model() const
    {
        return m_source_model;
    }

    void set_source_model(IObservableList<Data>* source_model)
    {
        if (m_source_model != source_model) {
            if (m_source_model) {
                m_source_model->template remove_listener<Biz::IListObserver<Data>>(this);
            }

            m_source_model = source_model;
            if (m_source_model) {
                m_source_model->template add_listener<Biz::IListObserver<Data>>(this);
            }

            on_reset();
        }
    }

private:
    void invalidate()
    {
        m_mapped_indexes.clear();

        if (!m_source_model) {
            this->template invoke_listeners<IListObserver<Data>>([&](auto* l) { l->on_reset(); });
            return;
        }

        m_mapped_indexes.reserve(m_source_model->size());

        std::unordered_set<GroupKey> group_keys_seen;

        for (size_t i = 0; i < m_source_model->size(); ++i) {
            const Data& source_item = m_source_model->at(i);

            if (m_filter_fn && !m_filter_fn(source_item)) {
                continue; // Skip filtered items
            }

            if (m_group_by_fn && m_group_by_fn(source_item, group_keys_seen)) {
                continue; // Skip subsequent items in the same group
            }

            m_mapped_indexes.push_back(i);
        }

        if (m_sort_fn) {
            std::sort(
                m_mapped_indexes.begin(),
                m_mapped_indexes.end(),
                [this](const std::optional<size_t>& lhs, const std::optional<size_t>& rhs) {
                return m_sort_fn(m_source_model->at(lhs.value()), m_source_model->at(rhs.value()));
            }
            );
        }

        this->template invoke_listeners<IListObserver<Data>>([&](auto* l) { l->on_reset(); });
    }

private:
    IObservableList<Data>* m_source_model{nullptr};
    std::vector<size_t> m_mapped_indexes;

    FilterFn m_filter_fn;
    SortFn m_sort_fn;
    GroupByFn m_group_by_fn;
};

} // namespace Slic3r::Biz
