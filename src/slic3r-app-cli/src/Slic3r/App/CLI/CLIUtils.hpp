#pragma once

#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"

using Slic3r::Domain::ConfigPack;
using Slic3r::Domain::ConfigPackFDM;
using Slic3r::Domain::ConfigPackSLA;
using Slic3r::Domain::Points;
using Slic3r::Domain::Vec2d;

namespace Slic3r::App::CLI {

inline static Domain::Points get_bed_shape(const Domain::ConfigPackFDM& config_pack)
{
    const std::vector<Vec2d> bed_shape =
        config_pack.printer.items.opt("bed_shape").get<std::vector<Vec2d>>();
    return Biz::Algorithms::Point::scaled(bed_shape);
}

inline static Domain::Points get_bed_shape(const Domain::ConfigPackSLA& config_pack)
{
    const std::vector<Vec2d> bed_shape =
        config_pack.sla_printer_settings.items.opt("bed_shape").get<std::vector<Vec2d>>();
    return Biz::Algorithms::Point::scaled(bed_shape);
}

inline Domain::Points get_bed_shape(const Domain::ConfigPack& config_pack)
{
    if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
        return get_bed_shape(std::get<Domain::ConfigPackFDM>(config_pack));
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
        return get_bed_shape(std::get<Domain::ConfigPackSLA>(config_pack));
    } else {
        PANIC("Unexpected config type!");
    }
}

inline static double min_object_distance([[maybe_unused]] const Domain::ConfigPackSLA& config_pack)
{
    return 6.;
}

inline static double min_object_distance(const Domain::ConfigPackFDM& config_pack)
{
    const double extruder_clearance_radius =
        config_pack.printer.items.opt("extruder_clearance_radius").get<double>();
    const double duplicate_distance =
        6.; // TODO: duplicate_distance was removed in the new configs.
    const bool complete_objects = config_pack.print.items.opt("complete_objects").get<bool>();

    // min object distance is max(duplicate_distance, clearance_radius)
    return (complete_objects && extruder_clearance_radius > duplicate_distance) ?
        extruder_clearance_radius :
        duplicate_distance;
}

inline double min_object_distance(const Domain::ConfigPack& config_pack)
{
    if (std::holds_alternative<Domain::ConfigPackFDM>(config_pack)) {
        return min_object_distance(std::get<Domain::ConfigPackFDM>(config_pack));
    } else if (std::holds_alternative<Domain::ConfigPackSLA>(config_pack)) {
        return min_object_distance(std::get<Domain::ConfigPackSLA>(config_pack));
    } else {
        PANIC("Unexpected config type!");
    }
}

}