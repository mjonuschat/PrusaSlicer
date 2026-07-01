#include "Slic3r/App/Plater/ToolGizmosUiInfo.hpp"
#include "Slic3r/App/Platform/CommandName.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Plater {

struct ToolGizmoUiInfo
{
    std::string name;
    const char* command_name;
    Platform::KeyCode key_code;
};

static const std::unordered_map<Scene::ToolType, ToolGizmoUiInfo> tool_gizmos_ui_info = {
    {Scene::ToolType::Translation,
     {Biz::L("Move"), Platform::CommandName::MoveGizmo, Platform::KeyCode::M}},
    {Scene::ToolType::Rotation,
     {Biz::L("Rotate"), Platform::CommandName::RotateGizmo, Platform::KeyCode::R}},
    {Scene::ToolType::Scale,
     {Biz::L("Scale"), Platform::CommandName::ScaleGizmo, Platform::KeyCode::S}},
    {Scene::ToolType::PlaceOnFace,
     {Biz::L("Place On Face"), Platform::CommandName::PlaceOnFace, Platform::KeyCode::F}},
    {Scene::ToolType::Simplify,
     {Biz::L("Simplify"), Platform::CommandName::SimplifyGizmo, Platform::KeyCode::E}},
    {Scene::ToolType::ArrangeGizmo,
     {Biz::L("Arrange"), Platform::CommandName::ArrangeGizmo, Platform::KeyCode::Q}},
    {Scene::ToolType::PaintOnSupportsGizmo,
     {Biz::L("Paint-on supports"),
      Platform::CommandName::PaintOnSupportsGizmo,
      Platform::KeyCode::L}},
    {Scene::ToolType::PaintOnSeamsGizmo,
     {Biz::L("Paint-on seams"), Platform::CommandName::PaintOnSeamsGizmo, Platform::KeyCode::P}},
    {Scene::ToolType::PaintOnFuzzySkinGizmo,
     {Biz::L("Paint-on fuzzy skin"),
      Platform::CommandName::PaintOnFuzzySkinGizmo,
      Platform::KeyCode::H}},
    {Scene::ToolType::MultiMaterialPaintingGizmo,
     {Biz::L("Multimaterial painting"),
      Platform::CommandName::MultiMaterialPaintingGizmo,
      Platform::KeyCode::N}},
    {Scene::ToolType::TextGizmo,
     {Biz::L("Emboss text"), Platform::CommandName::TextGizmo, Platform::KeyCode::T}},
    {Scene::ToolType::Svg,
     {Biz::L("Scalable vector graphics(SVG)"),
      Platform::CommandName::SvgGizmo,
      Platform::KeyCode::G}},
    {Scene::ToolType::MeasureGizmo,
     {Biz::L("Measure"), Platform::CommandName::MeasureGizmo, Platform::KeyCode::U}},
    {Scene::ToolType::CutGizmo,
     {Biz::L("Cut"), Platform::CommandName::CutGizmo, Platform::KeyCode::C}},
    {Scene::ToolType::VariableLayerHeightGizmo,
     {Biz::L("Variable Layer Height"),
      Platform::CommandName::VariableLayerHeightGizmo,
      Platform::KeyCode::V}},
    {Scene::ToolType::HeightRangeGizmo,
     {Biz::L("Height Range"), Platform::CommandName::HeightRangeGizmo, Platform::KeyCode::W}},
};

std::string tool_name(Scene::ToolType tool)
{
    ASSERT(
        tool_gizmos_ui_info.contains(tool),
        fmt::format("Tool({}) doesn't exist in tool_gizmos_ui_info", static_cast<int>(tool))
    );

    return Biz::_u8(tool_gizmos_ui_info.at(tool).name);
}

std::string tool_shortcut(Scene::ToolType tool)
{
    return Platform::to_string(tool_key_code(tool));
}

const char* tool_command_name(Scene::ToolType tool)
{
    ASSERT(
        tool_gizmos_ui_info.contains(tool),
        fmt::format("TooBiz::L({}) doesn't exist in tool_gizmos_ui_info", static_cast<int>(tool))
    );

    return tool_gizmos_ui_info.at(tool).command_name;
}

Platform::KeyCode tool_key_code(Scene::ToolType tool)
{
    ASSERT(
        tool_gizmos_ui_info.contains(tool),
        fmt::format("TooBiz::L({}) doesn't exist in tool_gizmos_ui_info", static_cast<int>(tool))
    );

    return tool_gizmos_ui_info.at(tool).key_code;
}
} // namespace Slic3r::App::Plater
