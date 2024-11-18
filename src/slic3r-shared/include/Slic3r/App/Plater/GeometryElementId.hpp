#pragma once

#include <boost/functional/hash.hpp>

namespace Slic3r::App::Plater {

struct GeometryElementId
{
    enum class Type : uint8_t
    {
        Volume = 0,
        Bed,
        WipeTower
    };

    Type type;
    size_t id;

    /**
     *
     * @param rhs
     * @return
     */
    bool operator==(const GeometryElementId& rhs) const { return type == rhs.type && id == rhs.id; }

    bool operator<(const GeometryElementId& rhs) const
    {
        return type < rhs.type || (type == rhs.type && id < rhs.id);
    }
};
} // namespace Slic3r::App::Plater

namespace std {
template<>
struct hash<Slic3r::App::Plater::GeometryElementId>
{
    using value_type = Slic3r::App::Plater::GeometryElementId;
    std::uint64_t operator()(const value_type& val) const
    {
        size_t ret = boost::hash_value(val.type);
        boost::hash_combine(ret, val.id);
        return ret;
    }
};
} // namespace std
