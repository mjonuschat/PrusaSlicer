#pragma once

#include <charconv>
#include <regex>
#include <vector>

#include <libslic3r/Point.hpp>
#include <libslic3r/Model.hpp>


namespace Slic3r::Tests {

struct Extrusion {
    Slic3r::Vec4d start;
    Slic3r::Vec4d end;
};

std::optional<std::string> is_gcode_sane(const std::string& gcode, const Slic3r::Model &model);

}
