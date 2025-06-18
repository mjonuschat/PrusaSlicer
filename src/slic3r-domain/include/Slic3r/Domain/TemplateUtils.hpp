#pragma once

#include <vector>
#include <optional>
#include <map>

namespace Slic3r::Domain {

template<typename T>
struct is_std_vector : std::false_type
{};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type
{};

template<typename T>
inline constexpr bool is_std_vector_v = is_std_vector<T>::value;

template<typename T>
struct is_std_optional : std::false_type
{};

template<typename T>
struct is_std_optional<std::optional<T>> : std::true_type
{};

template<typename T>
inline constexpr bool is_std_optional_v = is_std_optional<T>::value;

template<typename T>
struct is_std_map : std::false_type
{};

template<typename Key, typename T, typename Compare, typename Allocator>
struct is_std_map<std::map<Key, T, Compare, Allocator>> : std::true_type
{};

template<typename T>
inline constexpr bool is_std_map_v = is_std_map<T>::value;

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// deduction guide
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Slic3r::Domain
