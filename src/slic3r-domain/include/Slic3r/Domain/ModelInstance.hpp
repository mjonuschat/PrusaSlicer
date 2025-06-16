#pragma once

#include "Slic3r/Domain/Forward.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Transformation.hpp"

namespace cereal {
template<class Archive> void serialize(Archive&, Slic3r::Domain::ModelInstance&);
} // namespace cereal

namespace Slic3r::Domain {

enum ModelInstanceEPrintVolumeState : unsigned char
{
    ModelInstancePVS_Inside,
    ModelInstancePVS_Partly_Outside,
    ModelInstancePVS_Fully_Outside,
    ModelInstanceNum_BedStates
};

/**
 * A single instance of a ModelObject.
 * Knows the affine transformation of an object.
 */
class ModelInstance final : public ObjectBase
{
private:
    Transformation                 m_transformation;

    // Parent object, owning this instance.
    ModelObject*                   object;

public:
    // Flag showing the position of this instance with respect to the print volume (set by Print::validate() using ModelObject::check_instances_print_volume_state())
    ModelInstanceEPrintVolumeState print_volume_state;
    // Whether or not this instance is printable.
    bool                           printable { true };

    ModelObject*                  get_object() const;
    void                          set_model_object(ModelObject *model_object);

    const Transformation&         get_transformation() const;
    void                          set_transformation(const Transformation& transformation);

    Vec3d                         get_offset() const;
    double                        get_offset(Axis axis) const;

    void                          set_offset(const Vec3d& offset);
    void                          set_offset(Axis axis, double offset);

    Vec3d                         get_rotation() const;
    double                        get_rotation(Axis axis) const;

    void                          set_rotation(const Vec3d& rotation);
    void                          set_rotation(Axis axis, double rotation);

    Vec3d                         get_scaling_factor() const;
    double                        get_scaling_factor(Axis axis) const;

    void                          set_scaling_factor(const Vec3d& scaling_factor);
    void                          set_scaling_factor(Axis axis, double scaling_factor);

    Vec3d                         get_mirror() const;
    double                        get_mirror(Axis axis) const;

    void                          set_mirror(const Vec3d& mirror);
    void                          set_mirror(Axis axis, double mirror);

    const Transform3d&            get_matrix() const;
    Transform3d                   get_matrix_no_offset() const;
    bool                          is_left_handed() const;

    bool                          is_printable() const;

    void                          invalidate_object_bounding_box();

    ModelInstance(const ModelInstance& rhs) = default;

    bool operator==(const ModelInstance& rhs) const;

    explicit ModelInstance(ModelInstance&& rhs) = delete;
    ModelInstance& operator=(const ModelInstance& rhs) = delete;
    ModelInstance& operator=(ModelInstance&& rhs) = delete;

private:
    // Used for deserialization, therefore no IDs are allocated.
    ModelInstance() : ObjectBase(-1), object(nullptr) { assert(this->id().invalid()); }
    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject* object) : object(object), print_volume_state(Domain::ModelInstancePVS_Inside) { assert(this->id().valid()); }
    // Constructor, which assigns a new unique ID.
    explicit ModelInstance(ModelObject* object, const ModelInstance& other)
        : m_transformation(other.m_transformation), object(object), print_volume_state(Domain::ModelInstancePVS_Inside), printable(other.printable)
    {
        assert(this->id().valid() && this->id() != other.id());
    }

    friend class ModelObject;

    template<class Archive> friend void cereal::serialize(Archive&, ModelInstance&);
};

using ModelInstancePtrs = std::vector<ModelInstance*>;

} // namespace Slic3r::Domain
