#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/ScaleHelpers.hpp"
#include "imgui.h"

namespace Slic3r::App {

constexpr float imgui_scale_factor{18.0f / 14.0f};

float px(float value)
{
    return value * imgui_scale_factor;
}

ImVec2 px(const ImVec2& value)
{
    return {px(value.x), px(value.y)};
}

Domain::Vec2f px(const Domain::Vec2f& value)
{
    return {px(value.x()), px(value.y())};
}

float operator""_px(long double value)
{
    return px(static_cast<float>(value));
}

float operator""_px(unsigned long long value)
{
    return px(static_cast<float>(value));
}
} // namespace Slic3r::App::Plater
