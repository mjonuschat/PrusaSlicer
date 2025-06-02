///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Assert.hpp"

#include <cstddef>

namespace Slic3r::Biz {

/**
 * @note both from and to indexes are inclusive
 */
struct IndexRange
{
    IndexRange() = default;
    IndexRange(size_t index) : IndexRange(index, index) {}
    IndexRange(size_t from, size_t to) : from(from), to(to) { ASSERT(from <= to); }

    bool is_valid() const { return from <= to; }

    size_t from{0};
    size_t to{0};
};

template<class Data>
class IListObserver
{
public:
    /**
     * @brief on_inserted - Data at index was inserted
     */
    virtual void on_inserted(const Data& data, size_t index) = 0;
    /**
     * @brief on_removed - all Data in range [IndexRange.from, IndexRange.to] were removed
     */
    virtual void on_removed(const IndexRange& index_range) = 0;
    /**
     * @brief on_updated - add Data in range [IndexRange.from, IndexRange.to] were updated
     */
    virtual void on_updated(const IndexRange& index_range) = 0;
    /**
     * @brief on_reset - all Data is invalid, reconstruct List completely
     */
    virtual void on_reset() = 0;
    /**
     * @brief on_moved - Data from index was moved to to index
     */
    virtual void on_moved(size_t from, size_t to) = 0;
};

} // namespace Slic3r::Biz
