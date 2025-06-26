#pragma once

#include "Slic3r/Domain/Color.hpp"

#include <cstdint>

namespace Slic3r::App::Render {
class Material;
class Device;
} // Slic3r::App::Render

namespace Slic3r::Domain {
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {

/**
* @brief Bed colors
*/
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_MODEL_COLOR  = { 0.235f, 0.235f, 0.235f, 1.0f };
static const Slic3r::Domain::ColorRGBA DISABLED_BED_MODEL_COLOR = { 0.5f, 0.5f, 0.5f, 1.0f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_PLATE_COLOR  = { 0.225f, 0.225f, 0.225f, 1.0f };
static const Slic3r::Domain::ColorRGBA DISABLED_BED_PLATE_COLOR = { 0.425f, 0.425f, 0.425f, 1.0f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_GRID_COLOR  = { 0.75f, 0.75f, 0.75f, 0.75f };
static const Slic3r::Domain::ColorRGBA DISABLED_BED_GRID_COLOR = { 0.65f, 0.65f, 0.65f, 0.75f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_CONTOUR_COLOR  = { 0.9f, 0.9f, 0.9f, 1.0f };
static const Slic3r::Domain::ColorRGBA DISABLED_BED_CONTOUR_COLOR = { 0.75f, 0.75f, 0.75f, 1.0f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_X_AXIS_COLOR = { 0.75f, 0.0f, 0.0f, 1.0f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_Y_AXIS_COLOR = { 0.0f, 0.75f, 0.0f, 1.0f };
static const Slic3r::Domain::ColorRGBA DEFAULT_BED_Z_AXIS_COLOR = { 0.0f, 0.0f, 0.75f, 1.0f };

struct BedMaterials
{
    static Render::Material plate_default_material(const Render::Device& device);
    static Render::Material plate_textured_material(const Render::Device& device, const Domain::Bed& bed);
    static Render::Material grid_material(const Render::Device& device);
    static Render::Material contour_material(const Render::Device& device);
    static Render::Material print_volume_material(const Render::Device& device);
    static Render::Material model_material(const Render::Device& device);
    static Render::Material axis_material(const Render::Device& device, uint8_t axis);

    static Render::Material plate_default_override_material(const Render::Device& device);
    static Render::Material plate_textured_override_material(const Render::Material& primary_material);
    static Render::Material grid_override_material(const Render::Device& device);
    static Render::Material contour_override_material(const Render::Device& device);
    static Render::Material print_volume_override_material(const Render::Device& device);
    static Render::Material model_override_material(const Render::Device& device);
};

} // namespace Slic3r::App::Scene
