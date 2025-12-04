///|/ Copyright (c) Prusa Research 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelObject.hpp"

#include <vector>

namespace Slic3r {

struct ModelObjectCutAttributes {
    bool keep_upper{ true };
    bool keep_lower{ true };
    bool keep_as_parts{ true };
    bool flip_upper{ false };
    bool flip_lower{ false };
    bool place_on_cut_upper{ false };
    bool place_on_cut_lower{ false };
    bool create_dowels{ false };
    bool invalidate_cut_info{ false };
};

class Cut {
public:
    Cut(const Domain::ModelObject* object, int instance, const  Domain::Transform3d& cut_matrix,
        ModelObjectCutAttributes attributes = ModelObjectCutAttributes() );
    ~Cut() { m_model.clear_objects(); }

    struct Groove
    {
        float depth{ 0.f };
        float width{ 0.f };
        float flaps_angle{ 0.f };
        float angle{ 0.f };
        float depth_init{ 0.f };
        float width_init{ 0.f };
        float flaps_angle_init{ 0.f };
        float angle_init{ 0.f };
        float depth_tolerance{ 0.1f };
        float width_tolerance{ 0.1f };
    };

    struct Part
    {
        bool selected;
        bool is_modifier;
    };

    const Domain::ModelObjectPtrs& perform_with_plane();
    const Domain::ModelObjectPtrs& perform_by_contour(std::vector<Part> parts, int dowels_count);
    const Domain::ModelObjectPtrs& perform_with_groove(const Groove& groove, const  Domain::Transform3d& rotation_m, bool keep_as_parts = false);

private:
    void post_process(Domain::ModelObject* object, Domain::ModelObjectPtrs& objects, bool keep, bool place_on_cut, bool flip);
    void post_process(Domain::ModelObject* upper_object, Domain::ModelObject* lower_object, Domain::ModelObjectPtrs& objects);
    void finalize(const Domain::ModelObjectPtrs& objects);

private:
    Domain::Model               m_model;
    int                         m_instance;
    const  Domain::Transform3d  m_cut_matrix;
    ModelObjectCutAttributes    m_attributes;
}; // namespace Cut

} // namespace Slic3r

