#pragma once

#include <string>
#include <typeinfo>

namespace Slic3r {
namespace Internal { std::string demangle_name(const char* name); }

template <typename T>
std::string type_name(const T& t)
{
    return Internal::demangle_name(typeid(t).name());
}

}
