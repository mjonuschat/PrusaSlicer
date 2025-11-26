#pragma once

#include <Slic3r/Assert.hpp>
#include <type_traits>
#include <map>
#include <string>

namespace Slic3r::App::Wildcards {

enum class TypeFlag : int
{
    None        = 0,
    GCode       = 1 << 0,
    BinaryGCode = 1 << 1,
    Sl1         = 1 << 2,
    Sl1S        = 1 << 3,
    Stl         = 1 << 4,
    Project3mf  = 1 << 5,
    AllFlags    = 1 << 6
};

constexpr TypeFlag operator|(TypeFlag lhs, TypeFlag rhs)
{
    using underlying_type = std::underlying_type_t<TypeFlag>;
    
    return static_cast<TypeFlag>(
        static_cast<underlying_type>(lhs) | 
        static_cast<underlying_type>(rhs)
    );
}

constexpr TypeFlag operator&(TypeFlag lhs, TypeFlag rhs)
{
    using underlying_type = std::underlying_type_t<TypeFlag>;
    
    return static_cast<TypeFlag>(
        static_cast<underlying_type>(lhs) &
        static_cast<underlying_type>(rhs)
    );
}

inline TypeFlag& operator|=(TypeFlag& lhs, TypeFlag rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

/**
 * @brief Generates a pipe-separated wildcard string from `all_type`,
 * prioritizing the wildcard for `primary_type` (if set).
 */
std::string generate_wildcards(TypeFlag all_type, TypeFlag primary_type = TypeFlag::None);

}