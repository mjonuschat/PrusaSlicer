#include "Slic3r/Domain/GCode.hpp"

namespace Slic3r::Domain {

bool CustomGCodeItem::operator < (const CustomGCodeItem& other) const
{
    return this->print_z < other.print_z;
}

bool CustomGCodeItem::operator == (const CustomGCodeItem& other) const
{
    return other.print_z == this->print_z &&
           other.type == this->type &&
           other.extruder == this->extruder &&
           other.str_color == this->str_color &&
           other.extra == this->extra;
}

bool CustomGCodeItem::operator != (const CustomGCodeItem& other) const
{
    return !(*this == other);
}

bool CustomGCodeInfo::operator == (const CustomGCodeInfo& other) const
{
    if (other.gcodes.empty() && this->gcodes.empty())
        return true; // don't respect to the comparison of the mode, when g_codes are empty
    return other.mode == this->mode && other.gcodes == this->gcodes;
}

bool CustomGCodeInfo::operator != (const CustomGCodeInfo& other) const
{
    return !(*this == other);
}

} // namespace Slic3r::Domain
