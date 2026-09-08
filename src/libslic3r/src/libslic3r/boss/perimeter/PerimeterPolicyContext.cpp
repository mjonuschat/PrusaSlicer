#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

#include "libslic3r/PerimeterGenerator.hpp"

namespace Slic3r::Boss {

PerimeterPolicyContext make_perimeter_policy_context(const PerimeterGenerator::Parameters &params, size_t extruder_id)
{
    PerimeterPolicyContext ctx;
    ctx.layer_id       = params.layer_id;
    ctx.is_first_layer = params.layer_id == 0;
    ctx.spiral_vase    = params.spiral_vase;
    ctx.fill_density   = params.config.get<std::vector<Domain::Percentage>>("fill_density").at(extruder_id).value;
    ctx.extruder_id    = extruder_id;
    ctx.config         = &params.config;
    return ctx;
}

} // namespace Slic3r::Boss
