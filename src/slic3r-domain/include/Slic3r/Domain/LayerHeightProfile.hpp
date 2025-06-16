#pragma once

#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/ConfigBoxesFDM.hpp"

#include <map>

#include <cereal/types/base_class.hpp>

namespace Slic3r {
class ModelObject;
} // namespace Slic3r

namespace Slic3r::Domain {

class LayerHeightProfile final : public Domain::ObjectWithTimestamp
{
private:
    std::vector<double> m_data;

public:
    const std::vector<double>& get() const noexcept;
    bool                       empty() const noexcept;
    void                       set(const std::vector<double>& data);
    void                       set(std::vector<double>&& data);
    void                       clear();

    // Assign the content if the timestamp differs, don't assign an ObjectID.
    void assign(const LayerHeightProfile& rhs);
    void assign(LayerHeightProfile&& rhs);

private:
    // Constructors to be only called by derived classes.
    // Default constructor to assign a unique ID.
    explicit LayerHeightProfile() = default;
    // Constructor with ignored int parameter to assign an invalid ID, to be replaced
    // by an existing ID copied from elsewhere.
    explicit LayerHeightProfile(int) : ObjectWithTimestamp(-1) {}
    // Copy constructor copies the ID.
    LayerHeightProfile(const LayerHeightProfile& rhs) = default;
    // Move constructor copies the ID.
    LayerHeightProfile(LayerHeightProfile&& rhs) = default;

    // called by ModelObject::assign_copy()
    LayerHeightProfile& operator=(const LayerHeightProfile& rhs) = default;
    LayerHeightProfile& operator=(LayerHeightProfile&& rhs) = default;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<ObjectWithTimestamp>(this), m_data);
    }

    // To access set_new_unique_id() when copy / pasting an object.
    friend class Slic3r::ModelObject;
};

using LayerHeightRange = std::pair<double, double>;
using LayerConfigRanges = std::map<LayerHeightRange, VolumeSettings>;

} // namespace Slic3r::Domain
