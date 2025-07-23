///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <cstddef>

namespace Slic3r::Biz {

template<class Data>
class DataObserver
{
public:
    DataObserver(size_t index, const Data& data) : m_index(index), m_state(&data) {}
    virtual ~DataObserver() {}

    const Data* state() const {
        return m_state;
    }

    void set_state(const Data& state)
    {
        m_state = &state;
        on_data_update();
    }
    void set_state_index(size_t index, const Data& state)
    {
        m_state = &state;
        m_index = index;
        on_index_update();
        on_data_update();
    }

protected:
    virtual void on_data_update() {}
    virtual void on_index_update() {}

protected:
    size_t m_index = 0;
    const Data* m_state = nullptr;
};

} // namespace Slic3r::Biz
