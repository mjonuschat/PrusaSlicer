#pragma once

#include "Slic3r/Domain/Types.hpp"


namespace Slic3r::Domain {
// Note: The following class does not have to inherit from ObjectID, it is currently
// only used for arrangement. It might be good to refactor this in future.
class ModelWipeTower
{
public:
	Vec2d		position = Vec2d(180., 140.);
	double 		rotation = 0.;

    bool operator==(const ModelWipeTower& other) const { return position == other.position && rotation == other.rotation; }
    bool operator!=(const ModelWipeTower& other) const { return !((*this) == other); }

    // Assignment operator does not touch the ID!
    ModelWipeTower& operator=(const ModelWipeTower& rhs) { position = rhs.position; rotation = rhs.rotation; return *this; }

    explicit ModelWipeTower() {}
	explicit ModelWipeTower(const ModelWipeTower &cfg) = default;

    // For serialization / deserialization of ModelWipeTower composed into another class into the Undo / Redo stack as a separate object.
    template<typename Archive> void serialize(Archive &ar) { ar(position, rotation); }
};
}
