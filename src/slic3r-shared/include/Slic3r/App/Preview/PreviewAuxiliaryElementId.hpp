#pragma once

#include <boost/functional/hash.hpp>

namespace Slic3r::App::Preview {

struct PreviewAuxiliaryElementId
{
    enum class Type : uint8_t
    {
        Toolpaths = 0,
        CogMarker,
        ToolMarker,
        Bed,
    };

    Type type;
    size_t id;

    /**
     *
     * @param rhs
     * @return
     */
    bool operator==(const PreviewAuxiliaryElementId& rhs) const { return type == rhs.type && id == rhs.id; }

    bool operator<(const PreviewAuxiliaryElementId& rhs) const
    {
        return type < rhs.type || (type == rhs.type && id < rhs.id);
    }
};
} // namespace Slic3r::App::Preview

namespace std {
template<>
struct hash<Slic3r::App::Preview::PreviewAuxiliaryElementId>
{
    using value_type = Slic3r::App::Preview::PreviewAuxiliaryElementId;
    std::uint64_t operator()(const value_type& val) const
    {
        size_t ret = boost::hash_value(val.type);
        boost::hash_combine(ret, val.id);
        return ret;
    }
};
} // namespace std
