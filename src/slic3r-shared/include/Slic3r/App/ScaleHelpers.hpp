#include "Slic3r/Domain/Types.hpp"
#include "imgui.h"

namespace Slic3r::App {

float px(float value);

ImVec2 px(const ImVec2& value);

Domain::Vec2f px(const Domain::Vec2f& value);

float operator""_px(long double value);

float operator""_px(unsigned long long value);

} // namespace Slic3r::App::Plater
