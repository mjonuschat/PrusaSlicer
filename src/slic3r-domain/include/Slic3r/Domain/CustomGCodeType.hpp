#pragma once

#include <string>

namespace Slic3r::Domain {
enum CustomGCodeType
{
    ColorChange,
    PausePrint,
    ToolChange,
    Template,
    Custom
};

struct CustomGCodeItem
{
    bool operator<(const CustomGCodeItem& rhs) const { return this->print_z < rhs.print_z; }
    bool operator==(const CustomGCodeItem& rhs) const
    {
        return (rhs.print_z   == this->print_z    ) &&
               (rhs.type      == this->type       ) &&
               (rhs.extruder  == this->extruder   ) &&
               (rhs.color     == this->color      ) &&
               (rhs.extra     == this->extra      );
    }
    bool operator!=(const CustomGCodeItem& rhs) const { return ! (*this == rhs); }

    double      print_z;
    CustomGCodeType type;
    int         extruder;   // Informative value for ColorChangeCode and ToolChangeCode
                            // "gcode" == ColorChangeCode   => M600 will be applied for "extruder" extruder
                            // "gcode" == ToolChangeCode    => for whole print tool will be switched to "extruder" extruder
    std::string color;      // if gcode is equal to PausePrintCode, 
                            // this field is used for save a short message shown on Printer display 
    std::string extra;      // this field is used for the extra data like :
                            // - G-code text for the Type::Custom 
                            // - message text for the Type::PausePrint
};
}
