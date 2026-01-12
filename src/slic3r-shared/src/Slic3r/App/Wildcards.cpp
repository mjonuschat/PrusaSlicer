#include "Slic3r/App/Wildcards.hpp"

namespace Slic3r::App::Wildcards {

namespace {
const std::map<TypeFlag, std::string> flag_to_wildcard_map = {
    {TypeFlag::GCode, "G-code files (*.gcode)|*.gcode"},
    {TypeFlag::BinaryGCode, "Binary G-code files (*.bgcode)|*.bgcode"},
    {TypeFlag::Sl1, "SL1 files (*.sl1)|*.sl1"},
    {TypeFlag::Sl1S, "SL1S files (*.sl1s)|*.sl1s"},
    {TypeFlag::Stl, "STL files (*.stl)|*.stl"},
    {TypeFlag::Obj, "OBJ files (*.obj)|*.obj"},
    {TypeFlag::Project3mf, "3MF files (*.3mf)|*.3mf"},
    {TypeFlag::AllImportFiles, "All supported files|*.3mf;*.obj;*.stl"}
};
}

std::string generate_wildcards(TypeFlag all_flags, TypeFlag primary_flag /*= TypeFlag::None*/)
{
    std::string result_string;
    
    // Concatenate wildcards from map according to flags
    // First is primary
    if (primary_flag != TypeFlag::None) {
        auto it = flag_to_wildcard_map.find(primary_flag);
        ASSERT(it != flag_to_wildcard_map.end());
        result_string += it->second;
        if (all_flags == TypeFlag::None) {
            return result_string;
        }
    }

    // Concatenate rest of the wildcards
    using underlying_type = std::underlying_type_t<TypeFlag>;
    for (underlying_type flag_value = 1; 
         flag_value < static_cast<underlying_type>(TypeFlag::AllFlags); 
         flag_value <<= 1) 
    {
        TypeFlag single_flag = static_cast<TypeFlag>(flag_value);
        if (single_flag == primary_flag) {
            continue;
        }
        if ((all_flags & single_flag) != TypeFlag::None) {
            auto it = flag_to_wildcard_map.find(single_flag);
            ASSERT(it != flag_to_wildcard_map.end());
            if (!result_string.empty()) {
                 result_string += "|";
            }
            result_string += it->second;
        }
    }
    return result_string;
}
}