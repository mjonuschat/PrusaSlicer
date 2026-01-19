///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena
///|/
///|/ ported from lib/Slic3r/GUI/BedShapeDialog.pm:
///|/ Copyright (c) Prusa Research 2016 - 2018 Vojtěch Král @vojtechkral, Vojtěch Bubník @bubnikv
///|/ Copyright (c) 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) 2017 Ahmed Samir Abdelreheem @Samir55
///|/ Copyright (c) Slic3r 2014 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Bed.hpp"

namespace Slic3r::Biz::Config {

// struct to get needed information from the Domain::Bed

struct BedShape
{
    enum class Type
    {
        Rectangle = 0,
        Circle,
        Custom
    };

    enum class Parameter
    {
        RectSize,
        RectOrigin,
        Diameter
    };

    struct ParamAttributes
    {
        std::string name;
        std::string tooltip;
        int min;
        int max;
        int def_value;
    };

    static std::string get_type_name(Type type);
    static const ParamAttributes& attributes(Parameter param);

    BedShape(const std::vector<Domain::Vec2d>& points);

    bool is_custom() const;
    bool is_equal_to(const std::vector<Domain::Vec2d>& points) const;

    Type get_type() const;
    const std::vector<Domain::Vec2d>& contour() const;
    std::vector<Domain::Vec2d> triangles() const;
    Domain::Vec2d get_size() const;
    Domain::Vec2d get_origin() const;
    double get_diameter() const;

    std::string get_full_name_with_params() const;

private:
    Domain::Bed m_bed;
};

} // namespace Slic3r::Biz::Config
