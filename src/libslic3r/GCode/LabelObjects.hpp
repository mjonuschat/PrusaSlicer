#ifndef slic3r_GCode_LabelObjects_hpp_
#define slic3r_GCode_LabelObjects_hpp_

#include <string>
#include <vector>

#include "libslic3r/Print.hpp"

namespace Slic3r {

enum GCodeFlavor : unsigned char;
enum class LabelObjectsStyle;
struct PrintInstance;
class Print;
class GCodeWriter;

namespace GCode {

class LabelObjects
{
public:
    void init(const Print& print, LabelObjectsStyle label_object_style, GCodeFlavor gcode_flavor);
    std::string all_objects_header() const;
    std::string all_objects_header_singleline_json() const;

    bool update(const PrintInstance *instance);

    std::string maybe_start_instance(GCodeWriter& writer);

    std::string maybe_stop_instance();

    std::string maybe_change_instance(GCodeWriter& writer);

    bool has_active_instance();

private:
    struct LabelData
    {
        const PrintInstance* pi;
        std::string name;
        std::string center;
        std::string polygon;
        int unique_id;
    };

    // Geometry-only entries for regions that must never be printed inside an EXCLUDE_OBJECT_START/END
    // (or M486 S<id>) block, so a host's "Cancel Object" can never remove them: e.g. the skirt/brim
    // and wipe tower. Only populated for the Klipper firmware flavor, the only one of the supported
    // protocols where defining an object's geometry is decoupled from marking it excludable.
    struct StaticLabelData
    {
        std::string name;
        std::string center;
        std::string polygon;
    };

    enum class IncludeName {
        No,
        Yes
    };

    std::string start_object(const PrintInstance& print_instance, IncludeName include_name) const;
    std::string stop_object(const PrintInstance& print_instance) const;

    const PrintInstance* current_instance{nullptr};
    const PrintInstance* last_operation_instance{nullptr};

    LabelObjectsStyle m_label_objects_style;
    GCodeFlavor       m_flavor;
    std::vector<LabelData> m_label_data;
    std::vector<StaticLabelData> m_static_label_data;
};
} // namespace GCode
} // namespace Slic3r

#endif // slic3r_GCode_LabelObjects_hpp_
