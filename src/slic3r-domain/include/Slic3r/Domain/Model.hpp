#pragma once

#include "Slic3r/Domain/Types.hpp"


namespace Slic3r::Domain {
// Note: The following class does not have to inherit from ObjectID, it is currently
// only used for arrangement. It might be good to refactor this in future.
class ModelWipeTower
{
public:
    Vec2d position{180., 140.};
    double rotation{};

    bool operator==(const ModelWipeTower& other) const
    {
        return position == other.position && rotation == other.rotation;
    }
    bool operator!=(const ModelWipeTower& other) const { return !((*this) == other); }

    // For serialization / deserialization of ModelWipeTower composed into another class into the
    // Undo / Redo stack as a separate object.
    template<typename Archive>
    void serialize(Archive& ar)
    {
        ar(position, rotation);
    }
};
}
