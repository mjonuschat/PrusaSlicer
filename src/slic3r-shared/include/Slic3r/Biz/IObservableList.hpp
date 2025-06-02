///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/IListObserver.hpp"

#include "Slic3r/Biz/Platform/WithListeners.hpp"

namespace Slic3r::Biz {

template<class Data>
class IObservableList : public WithListeners<IListObserver<Data>>
{
public:
    /**
     * @return const reference to element at index
     */
    virtual const Data& at(size_t index) const = 0;

    /**
     * @return size of items in list
     */
    virtual size_t size() const = 0;
};

} // namespace Slic3r::Biz
