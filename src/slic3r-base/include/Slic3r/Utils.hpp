#pragma once

#include <vector>

namespace Slic3r {

template<typename T, typename Alloc, typename Alloc2>
inline void append(std::vector<T, Alloc>& dest, const std::vector<T, Alloc2>& src)
{
    if (dest.empty()) {
        dest = src; // Copy
    } else {
        dest.insert(dest.end(), src.begin(), src.end());
    }
}

template<typename T, typename Alloc>
inline void append(std::vector<T, Alloc>& dest, std::vector<T, Alloc>&& src)
{
    if (dest.empty()) {
        dest = std::move(src);
    } else {
        dest.insert(dest.end(), std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));

        // Release memory of the source contour now.
        src.clear();
        src.shrink_to_fit();
    }
}

} // namespace Slic3r
