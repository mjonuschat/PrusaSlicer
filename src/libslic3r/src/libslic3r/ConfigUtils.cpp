#include "libslic3r/ConfigUtils.hpp"
#include <boost/algorithm/string/predicate.hpp>

namespace Slic3r {
std::string get_extrusion_axis(const PrintConfigView& cfg)
{
    using Domain::GCodeFlavor;
    return ((cfg.get<GCodeFlavor>("gcode_flavor") == GCodeFlavor::gcfMach3)
            || (cfg.get<GCodeFlavor>("gcode_flavor") == GCodeFlavor::gcfMachinekit)) ?
        "A" :
        (cfg.get<GCodeFlavor>("gcode_flavor") == GCodeFlavor::gcfNoExtrusion) ? "" :
                                                                                "E";
}

static bool is_XL_printer(const std::string& printer_notes)
{
    return boost::algorithm::contains(printer_notes, "PRINTER_VENDOR_PRUSA3D")
        && boost::algorithm::contains(printer_notes, "PRINTER_MODEL_XL");
}

bool is_XL_printer(const PrintConfigView &cfg)
{
    return is_XL_printer(cfg.get<std::string>("printer_notes"));
}
} // namespace Slic3r
