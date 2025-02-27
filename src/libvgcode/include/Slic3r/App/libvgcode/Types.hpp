///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Biz/libpgcode/Types.hpp>

#include <libslic3r/format.hpp>

namespace Slic3r::App::libvgcode {

static constexpr double INV255 = 1.0 / 255.0;

//
// Predefined values for the radius, in mm, of the cylinders used to render the travel moves.
//
static constexpr float DEFAULT_TRAVELS_RADIUS_MM = 0.1f;
static constexpr float MIN_TRAVELS_RADIUS_MM = 0.05f;
static constexpr float MAX_TRAVELS_RADIUS_MM = 1.0f;

//
// Predefined values for the radius, in mm, of the cylinders used to render the wipe moves.
//
static constexpr float DEFAULT_WIPES_RADIUS_MM = 0.1f;
static constexpr float MIN_WIPES_RADIUS_MM = 0.05f;
static constexpr float MAX_WIPES_RADIUS_MM = 1.0f;

//
// Predefined colors
//
static const ColorRGB DUMMY_COLOR = { 0.25f, 0.25f, 0.25f };

//
// Color palette
//
using Palette = std::vector<ColorRGB>;

//
// One dimensional natural numbers interval
// [0] -> min
// [1] -> max
//
using Interval = std::array<size_t, 2>;

//
// View types
//
enum class ViewType : uint8_t
{
    FeatureType,
    Height,
    Width,
    Speed,
    ActualSpeed,
    FanSpeed,
    Temperature,
    VolumetricFlowRate,
    ActualVolumetricFlowRate,
    LayerTimeLinear,
    LayerTimeLogarithmic,
    Tool,
    ColorPrint,
    COUNT
};

static constexpr size_t VIEW_TYPES_COUNT = size_t(ViewType::COUNT);

typedef std::function<void(bool)> CustomOptionActionCallback;

struct CustomOption
{
    std::string name;
    wchar_t icon;
    bool visible{ false };
    CustomOptionActionCallback cb_action{ nullptr };
};

using CustomOptions = std::vector<CustomOption>;

enum class LightReferenceSystem
{
    World,
    Eye,
    COUNT
};

static constexpr size_t LIGHT_REFERENCE_SYSTEMS_COUNT = size_t(LightReferenceSystem::COUNT);

std::string light_reference_system_to_string(LightReferenceSystem sys);

static constexpr LightReferenceSystem DEFAULT_LIGHT_REFERENCE_SYSTEMS = LightReferenceSystem::World;
static const Vec3f DEFAULT_LIGHT_DIRECTION = { 0.0f, 0.0f, -1.0f };
static constexpr float DEFAULT_LIGHT_AMBIENT = 0.1f;
static constexpr float DEFAULT_LIGHT_DIFFUSE = 0.1f;
static constexpr float DEFAULT_LIGHT_SPECULAR = 0.1f;
static constexpr float DEFAULT_LIGHT_SHININESS = 1.0f;

struct Light
{
    LightReferenceSystem system{ DEFAULT_LIGHT_REFERENCE_SYSTEMS };
    Vec3f direction{ DEFAULT_LIGHT_DIRECTION };
    float ambient{ DEFAULT_LIGHT_AMBIENT };
    float diffuse{ DEFAULT_LIGHT_DIFFUSE };
    float specular{ DEFAULT_LIGHT_SPECULAR };
    float shininess{ DEFAULT_LIGHT_SHININESS };
};

static constexpr size_t MAX_NUM_LIGHTS = 4;

using Lights = std::vector<Light>;

//
// Parameters for export to obj file
//
struct ObjExportParams
{
    std::string app_name;
    std::string app_version;
    std::string materials_filename;
    float cap_rounding_factor{ 0.25f };
};

struct ColorPrint
{
    uint8_t extruder_id{ 0 };
    uint8_t color_id{ 0 };
    uint32_t layer_id{ 0 };
    Biz::libpgcode::Times times{};
};

using ColorPrints = std::vector<ColorPrint>;

} // namespace Slic3r::App::libvgcode
