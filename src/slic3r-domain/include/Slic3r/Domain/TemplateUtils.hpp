#pragma once

#include <vector>

namespace Slic3r::Domain {

template<typename T>
struct is_std_vector : std::false_type
{};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type
{};

template<typename T>
inline constexpr bool is_std_vector_v = is_std_vector<T>::value;
} // namespace Slic3r::Domain
