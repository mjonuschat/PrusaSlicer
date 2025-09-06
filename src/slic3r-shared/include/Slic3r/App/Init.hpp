#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Percentage.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace Slic3r::App {

enum class ActionType
{
    HelpFFF,
    HelpSLA,
    ModelInfo,
    ConfigurationSave,
    QueryPrinterModels,
    QueryPrintToolFilamentProfiles,
    Slice,
    ExportSTL,
    ExportOBJ,
    Export3MF,
    ExportGCode,
    ExportSLA,
    GCodeViewer
};

struct InputParams
{
    std::vector<std::string> input_files; // Input files or URLs
    std::vector<std::string> config_files;
    std::vector<std::string> material_profile_presets;
    std::vector<std::string> tool_profile_presets;

    std::optional<std::string> print_profile_preset;
    std::optional<std::string> printer_profile_preset;
};

struct TransformParams
{
    std::optional<Domain::Vec2d> align_xy;
    std::optional<Domain::Vec2d> center;
    std::optional<double> cut_z;
    std::optional<bool> dont_arrange;

    std::optional<uint32_t> duplicate;
    std::optional<std::array<uint32_t, 2>> duplicate_grid;

    std::optional<bool> ensure_on_bed;
    std::optional<bool> merge;

    std::optional<Domain::Vec3d> rotation;

    std::optional<Domain::FloatOrPercentage> scale;
    std::optional<Domain::Vec3d> scale_to_fit;

    std::optional<bool> split;
};

struct MiscParams
{
    std::optional<std::string> datadir;
    std::optional<uint8_t> loglevel;
    std::optional<std::string> output;

    std::optional<bool> delete_after_load;
    std::optional<bool> ignore_nonexistent_config;
    std::optional<bool> single_instance;
    std::optional<bool> webdev;
    std::optional<uint16_t> threads;

    std::optional<bool> opengl_aa;
    std::optional<bool> opengl_compatibility;
    std::optional<bool> opengl_debug;
    std::optional<std::pair<int, int>> opengl_version;
    std::optional<bool> sw_renderer;
};

class InitParams final
{
public:
    InitParams() = default;
    InitParams(int argc, char** argv);

    std::optional<ActionType> action;

    InputParams input;
    TransformParams transform;
    MiscParams misc;

    std::vector<Domain::ConfigItem> config_overrides;

    int argc    = 0;
    char** argv = nullptr;

    std::optional<int> exit_code;
};

void init_paths(const InitParams& init_params);

void init_common();

} // namespace Slic3r::App
