#include "Slic3r/App/Yoga/LayerHeightProfileControl.hpp"

#include <cmath>
#include <imgui/imgui.h>

using Slic3r::Domain::ZHeightPair;
using Slic3r::Domain::ZHeightPairs;

const constexpr float LAYER_HEIGHT_BASELINE_THICKNESS = 2.f;
const constexpr float LAYER_HEIGHT_PROFILE_THICKNESS  = 2.f;
const constexpr ImColor LAYER_HEIGHT_PROFILE_COLOR    = ImColor(175, 119, 255, 255);
const constexpr ImColor LAYER_HEIGHT_BASELINE_COLOR   = ImColor(0, 0, 0, 255);

namespace Slic3r::App::Yoga {

LayerHeightProfileControl::LayerHeightProfileControl() : Item()
{
    this->set_object_name("LayerHeightProfileControl");
    this->set_flex_grow(1.f);
}

void LayerHeightProfileControl::set_object_max_z(const float object_max_z)
{
    m_object_max_z = object_max_z;
}

void LayerHeightProfileControl::set_min_layer_height(const float min_layer_height)
{
    m_min_layer_height = min_layer_height;
}

void LayerHeightProfileControl::set_max_layer_height(const float max_layer_height)
{
    m_max_layer_height = max_layer_height;
}

void LayerHeightProfileControl::set_default_layer_height(const float default_layer_height)
{
    m_default_layer_height = default_layer_height;
}

void LayerHeightProfileControl::set_layer_height_profile(const ZHeightPairs& layer_height_profile)
{
    m_layer_height_profile = layer_height_profile;
}

float LayerHeightProfileControl::object_max_z() const
{
    return m_object_max_z;
}

float LayerHeightProfileControl::min_layer_height_value() const
{
    return m_min_layer_height;
}

float LayerHeightProfileControl::max_layer_height_value() const
{
    return m_max_layer_height;
}

float LayerHeightProfileControl::project_layer_height(
    float layer_height,
    float out_range_min,
    float out_range_max
) const
{
    ASSERT(m_min_layer_height < m_max_layer_height);
    const float t = std::clamp(
        (layer_height - m_min_layer_height) / (m_max_layer_height - m_min_layer_height),
        0.f,
        1.f
    );
    return std::lerp(out_range_min, out_range_max, t);
}

float LayerHeightProfileControl::project_layer_z(
    float layer_z,
    float out_range_min,
    float out_range_max
) const
{
    ASSERT(m_min_layer_height < m_max_layer_height);
    const float t = std::clamp(1.f - (layer_z / m_object_max_z), 0.f, 1.f);
    return std::lerp(out_range_min, out_range_max, t);
}

void LayerHeightProfileControl::render_baseline(const Vec2f& pos, const Vec2f& size) const
{
    if (m_min_layer_height == m_max_layer_height) {
        return;
    }

    const float baseline_pos_x =
        this->project_layer_height(m_default_layer_height, pos.x(), pos.x() + size.x());

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddLine(
        ImVec2(baseline_pos_x, pos.y()),
        ImVec2(baseline_pos_x, pos.y() + size.y()),
        LAYER_HEIGHT_BASELINE_COLOR,
        LAYER_HEIGHT_BASELINE_THICKNESS
    );
}

void
LayerHeightProfileControl::render_layer_height_profile(const Vec2f& pos, const Vec2f& size) const
{
    if (m_min_layer_height == m_max_layer_height) {
        return;
    }

    std::vector<ImVec2> profile_points;
    profile_points.reserve(m_layer_height_profile.size());

    for (const ZHeightPair& layer : m_layer_height_profile) {
        const float layer_z      = static_cast<float>(layer.z);
        const float layer_height = static_cast<float>(layer.layer_height);

        const float layer_z_pos_y = this->project_layer_z(layer_z, pos.y(), pos.y() + size.y());
        const float layer_height_pos_x =
            this->project_layer_height(layer_height, pos.x(), pos.x() + size.x());

        profile_points.emplace_back(layer_height_pos_x, layer_z_pos_y);
    }

    if (profile_points.empty()) {
        const float profile_pos_x =
            this->project_layer_height(m_default_layer_height, pos.x(), pos.x() + size.x());
        profile_points.emplace_back(profile_pos_x, pos.y());
        profile_points.emplace_back(profile_pos_x, pos.y() + size.y());
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddPolyline(
        profile_points.data(),
        static_cast<int>(profile_points.size()),
        LAYER_HEIGHT_PROFILE_COLOR,
        ImDrawFlags_None,
        LAYER_HEIGHT_PROFILE_THICKNESS
    );
}

} // namespace Slic3r::App::Yoga
