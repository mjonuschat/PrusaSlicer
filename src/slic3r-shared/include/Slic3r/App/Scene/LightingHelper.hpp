#pragma once

#include <functional>

namespace Slic3r::App::Render {
class Material;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {

struct Lighting;
class ISceneProvider;

struct PBRParams
{
    float metal{ DEFAULT_METAL };
    float roughness{ DEFAULT_ROUGHNESS };
    float ior{ DEFAULT_IOR };

    static constexpr float DEFAULT_METAL = 0.0f;
    static constexpr float DEFAULT_ROUGHNESS = 0.25f;
    static constexpr float DEFAULT_IOR = 1.5f;
};

static const PBRParams DEFAULT_VOLUME_PBRPARAMS = { 0.0f, 0.25f, 1.5f };
static const PBRParams DEFAULT_BED_PLATE_PBRPARAMS = { 0.5f, 0.75f, 1.5f };
static const PBRParams DEFAULT_BED_MODEL_PBRPARAMS = { 0.5f, 0.75f, 1.5f };
static const PBRParams DEFAULT_GCODE_SEGMENTS_PBRPARAMS = { 0.0f, 0.25f, 1.5f };
static const PBRParams DEFAULT_GCODE_OPTIONS_PBRPARAMS = { 0.0f, 0.25f, 1.5f };

void set_uniforms(const Lighting& lights, Render::Material& material);
void set_uniforms(const PBRParams& pbr, Render::Material& material);

void render_imgui_scene_shading_customization(ISceneProvider& scene_provider, std::function<void(void)> cb_update_beds_shadows_data = nullptr);
void render_imgui_lights_customization(ISceneProvider& scene_provider);

} // namespace Slic3r::App::Scene
