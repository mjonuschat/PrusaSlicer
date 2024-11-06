#pragma once

#include <vector>
#include <tuple>

namespace Slic3r::Domain
{
    struct ElementRef
{
        size_t object_id{0};
        size_t instance_id{0};
        size_t volume_id{0};

        bool operator==(const ElementRef& rhs) const
        { return as_tuple() == rhs.as_tuple(); }

        bool operator!=(const ElementRef& rhs) const
        { return as_tuple() != rhs.as_tuple(); }

        bool operator<(const ElementRef& rhs) const
        { return as_tuple() < rhs.as_tuple(); }

        bool operator>(const ElementRef& rhs) const
        { return as_tuple() > rhs.as_tuple(); }

        bool operator<=(const ElementRef& rhs) const
        { return as_tuple() <= rhs.as_tuple(); }

        bool operator>=(const ElementRef& rhs) const
        { return as_tuple() >= rhs.as_tuple(); }

        bool valid() const { return object_id != 0; }
        bool has_instance() const { return instance_id != 0; }
        bool has_volume() const { return volume_id != 0; }
    private:
        constexpr std::tuple<size_t, size_t, size_t> as_tuple() const
        {
            return std::tie(object_id, instance_id, volume_id);
        }
    };

    using ElementRefs = std::vector<ElementRef>;
}
