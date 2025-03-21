#pragma once

#include <variant>
#include <string>
#include <map>
#include <vector>

#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Preset/Types.hpp"

namespace Slic3r::Domain::Preset {


/**
 * @brief Address of feeder or material slot.
 *
 * The address describes position of particular element in feeder topology.
 *
 * Examples:
 * - `{0}`: denotes first tool slot
 * - `{2,0}`: denotes a first material slot in a feeder attached to the 3rd tool head.
 * .
 */
using Address = std::vector<uint8_t>;

/**
 * @brief Model information of a hardware component (like printer or feeder).
 */
struct HwModel
{
    std::string model;
    std::string base_model;
};

/**
 * @brief Type of tool head.
 */
enum class ToolType : uint8_t
{
    FdmPrintTool,
};

/**
 * @brief Tool head configuration
 */
struct HwToolConfig
{
    // TODO: To decide: if we move this into `features`, this struct becomes more generic
    // (i.e. more suitable for SLA)
    float nozzle_diameter;
    PresetValueMap features;
};

/**
 * @brief Type of feeder
 */
enum class FeederType : uint8_t
{
    Manual, ///< User feel feed filaments manually on prompt.
    MMU
};

struct HwFeederConfig
{
    std::string id;
    FeederType type;
    HwModel model;
    uint32_t slot_count{0};
    PresetValueMap features;
};

struct MaterialConfig
{
    PresetValueMap features;
};

using HwToolConfigs = std::vector<HwToolConfig>;
using HwFeederConfigs = std::map<Address, HwFeederConfig>;
using HwMaterialConfigs = std::map<Address, MaterialConfig>;

struct HwPrinterConfig
{
    std::string id;
    PrinterTechnology technology;
    HwModel model;
    uint8_t tool_count;
    PresetValueMap features;

    HwToolConfigs tools;
    HwFeederConfigs feeders;
    HwMaterialConfigs materials;
};

struct FeatureDef
{
    std::string name;
    std::optional<PresetValue> default_value;
    std::vector<PresetValue> allowed_values;
    bool user_editable{true};
};

using FeatureDefs = std::vector<FeatureDef>;

struct HwPrinterConfigDef
{
    std::string id;
    PrinterTechnology technology;
    HwModel model;
    FeatureDefs features;
};

struct HwToolConfigDef
{
    std::string id;
    PrinterTechnology technology;
    std::optional<std::string> condition;
    FeatureDefs features;
};

struct HwFeederConfigDef
{
    std::string id;
    PrinterTechnology technology;
    std::optional<std::string> condition;
    HwModel model;
    FeatureDefs features;
};

struct HwPrinterTechnologyDefs
{
    PrinterTechnology technology;
    std::map<std::string, HwPrinterConfigDef> printers;
    std::map<std::string, HwToolConfigDef> tools;
    std::map<std::string, HwFeederConfigDef> feeders;
};

using HwDefs = std::map<PrinterTechnology, HwPrinterTechnologyDefs>;

} // namespace Slic3r::Domain::Presets
