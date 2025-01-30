#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace Slic3r::Domain {

enum class PrinterMode
{
    Undefined,
    SingleExtruder, // Single extruder printer preset is selected
    MultiAsSingle,  // Multiple extruder printer preset is selected, but 
                    // this mode works just for Single extruder print 
                    // (The same extruder is assigned to all ModelObjects and ModelVolumes).
    MultiExtruder,  // Multiple extruder printer preset is selected
    COUNT
};

static constexpr size_t PRINTER_MODES_COUNT = size_t(PrinterMode::COUNT);

enum class CustomGCodeType : uint8_t
{
    ColorChange,
    PausePrint,
    ToolChange,
    Template,
    Custom,
    COUNT
};

static constexpr size_t CUSTOM_GCODE_TYPES_COUNT = size_t(CustomGCodeType::COUNT);

struct CustomGCodeItem
{
    float print_z{ 0.0f };
    CustomGCodeType type{ CustomGCodeType::Custom };
    // Informative value for ColorChangeCode and ToolChangeCode
    // "gcode" == ColorChangeCode   => M600 will be applied for "extruder" extruder
    // "gcode" == ToolChangeCode    => for whole print tool will be switched to "extruder" extruder
    int8_t extruder{ -1 };
    // if gcode is equal to PausePrintCode, 
    // this field is used for save a short message shown on Printer display 
    std::string str_color;
    // this field is used for the extra data like :
    // - G-code text for the Type::Custom 
    // - message text for the Type::PausePrint
    std::string extra;

    bool operator < (const CustomGCodeItem& other) const;
    bool operator == (const CustomGCodeItem& other) const;
    bool operator != (const CustomGCodeItem& other) const;
};

struct CustomGCodeInfo
{
    PrinterMode mode{ PrinterMode::Undefined };
    std::vector<CustomGCodeItem> gcodes;

    bool operator == (const CustomGCodeInfo& other) const;
    bool operator != (const CustomGCodeInfo& other) const;
};

} // namespace Slic3r::Domain
