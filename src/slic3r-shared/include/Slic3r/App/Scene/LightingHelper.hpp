#pragma once

#include "Slic3r/App/Render/ImguiRender.hpp"

#include <functional>
#include <vector>

namespace Slic3r::App::Render {
class Material;
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Domain {
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {

struct Lighting;
class ISceneProvider;

struct PBRParams
{
    float metal{ DEFAULT_METAL };
    float roughness{ DEFAULT_ROUGHNESS };
    float ior{ DEFAULT_IOR };

    bool operator == (const PBRParams& other) const {
        return metal == other.metal && roughness == other.roughness && ior == other.ior;
    }

    static constexpr float DEFAULT_METAL = 0.0f;
    static constexpr float DEFAULT_ROUGHNESS = 0.25f;
    static constexpr float DEFAULT_IOR = 1.5f;
};

static const PBRParams DEFAULT_VOLUME_PBRPARAMS = { 0.0f, 0.25f, 1.5f };
static const PBRParams DEFAULT_SLA_SUPPORTS_PBRPARAMS = { 0.0f, 0.5f, 1.4f };
static const PBRParams DEFAULT_SLA_PAD_PBRPARAMS = { 0.0f, 0.5f, 1.4f };
static const PBRParams DEFAULT_BED_PLATE_PBRPARAMS = { 0.5f, 0.75f, 1.5f };
static const PBRParams DEFAULT_BED_MODEL_PBRPARAMS = { 0.5f, 0.75f, 1.5f };
static const PBRParams DEFAULT_GCODE_SEGMENTS_PBRPARAMS = { 0.0f, 0.5f, 1.5f };
static const PBRParams DEFAULT_GCODE_OPTIONS_PBRPARAMS = { 0.0f, 0.2f, 2.0f };

using PBRParamsList = std::vector<PBRParams>;

// The following value must match MAX_MATERIALS defined into ao_lighting.fs shader
static constexpr size_t MAX_NUM_PBR_MATERIALS = 16;

void set_uniforms(const Lighting& lights, Render::Material& material);
void set_uniforms(const PBRParamsList& pbr_params, Render::Material& material);

void render_imgui_graphics_settings_debug_window(const Domain::Project& project, const Render::Device& device, ISceneProvider& scene_provider,
    Render::ImguiRender& imgui_render);

} // namespace Slic3r::App::Scene
