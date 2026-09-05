#ifndef slic3r_Fill_Fill_hpp_
#define slic3r_Fill_Fill_hpp_

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "libslic3r/ExtrusionRole.hpp"
#include "libslic3r/Flow.hpp"

namespace Slic3r {

struct SurfaceFillParams
{
    // Zero based extruder ID.
    unsigned int 	extruder = 0;
    // Infill pattern, adjusted for the density etc.
    Domain::InfillPattern  	pattern = Domain::InfillPattern(0);

    // FillBase
    // in unscaled coordinates
    double    	spacing = 0.;
    // Angle as provided by the region config, in radians.
    float       	angle = 0.f;
    // Is bridging used for this fill? Bridging parameters may be used even if this->flow.bridge() is not set.
    bool 			bridge;
    // Non-negative for a bridge.
    float 			bridge_angle = 0.f;

    // FillParams
    float       	density = 0.f;
    // Length of the infill anchor along the perimeter line.
    // 1000mm is roughly the maximum length line that fits into a 32bit coord_t.
    float 			anchor_length     = 1000.f;
    float 			anchor_length_max = 1000.f;

    // width, height of extrusion, nozzle diameter, is bridge
    // For the output, for fill generator.
    Flow 			flow;

    // For the output
    ExtrusionRole	extrusion_role{ ExtrusionRole::None };
    float           role_speed = 0.f;

    // Index of this entry in a linear vector.
    size_t 			idx = 0;

    bool operator<(const SurfaceFillParams &rhs) const {
#define RETURN_COMPARE_NON_EQUAL(KEY) if (this->KEY < rhs.KEY) return true; if (this->KEY > rhs.KEY) return false;
#define RETURN_COMPARE_NON_EQUAL_TYPED(TYPE, KEY) if (TYPE(this->KEY) < TYPE(rhs.KEY)) return true; if (TYPE(this->KEY) > TYPE(rhs.KEY)) return false;

        // Sort first by decreasing bridging angle, so that the bridges are processed with priority when trimming one layer by the other.
        if (this->bridge_angle > rhs.bridge_angle) return true;
        if (this->bridge_angle < rhs.bridge_angle) return false;

        RETURN_COMPARE_NON_EQUAL(extruder);
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, pattern);
        RETURN_COMPARE_NON_EQUAL(spacing);
        RETURN_COMPARE_NON_EQUAL(angle);
        RETURN_COMPARE_NON_EQUAL(density);
        RETURN_COMPARE_NON_EQUAL(anchor_length);
        RETURN_COMPARE_NON_EQUAL(anchor_length_max);
        RETURN_COMPARE_NON_EQUAL(flow.width());
        RETURN_COMPARE_NON_EQUAL(flow.height());
        RETURN_COMPARE_NON_EQUAL(flow.nozzle_diameter());
        RETURN_COMPARE_NON_EQUAL_TYPED(unsigned, bridge);
        if (this->extrusion_role.lower(rhs.extrusion_role)) return true;
        if (rhs.extrusion_role.lower(this->extrusion_role)) return false;
        RETURN_COMPARE_NON_EQUAL(role_speed);
        return false;

#undef RETURN_COMPARE_NON_EQUAL
#undef RETURN_COMPARE_NON_EQUAL_TYPED
    }

    bool operator==(const SurfaceFillParams &rhs) const {
        return  this->extruder 			== rhs.extruder 		&&
                this->pattern 			== rhs.pattern 			&&
                this->spacing 			== rhs.spacing 			&&
                this->angle   			== rhs.angle   			&&
                this->bridge   			== rhs.bridge   		&&
                this->density   		== rhs.density   		&&
                this->anchor_length  	== rhs.anchor_length    &&
                this->anchor_length_max == rhs.anchor_length_max &&
                this->flow 				== rhs.flow 			&&
                this->extrusion_role	== rhs.extrusion_role	&&
                this->role_speed        == rhs.role_speed;
    }
};

} // namespace Slic3r

#endif // slic3r_Fill_Fill_hpp_
