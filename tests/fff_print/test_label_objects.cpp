#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Print.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using namespace Test;

namespace {

void add_object(Model &model, const std::string &name, const int extruder, const Vec3d &offset = Vec3d::Zero())
{
    std::string extruder_id{std::to_string(extruder)};
    ModelObject *object = model.add_object();
    object->name = name;
    ModelVolume *volume = object->add_volume(Test::mesh(Test::TestMesh::cube_20x20x20));
    volume->set_material_id("material" + extruder_id);
    volume->translate(offset);
    DynamicPrintConfig config;
    config.set_deserialize_strict({{"extruder", extruder_id}});
    volume->config.assign_config(config);
    object->add_instance();
    object->ensure_on_bed();
}

} // namespace

TEST_CASE("Skirt and wipe tower are declared but never wrapped as excludable under Klipper firmware labeling", "[LabelObjects]")
{
    DynamicPrintConfig config{Slic3r::DynamicPrintConfig::full_print_config()};
    config.set_deserialize_strict({{"nozzle_diameter", "0.4,0.4"}});
    config.normalize_fdm();
    config.set_deserialize_strict({
        {"gcode_flavor", "klipper"},
        {"gcode_label_objects", "firmware"},
        {"gcode_comments", "1"},
        {"wipe_tower", "1"},
        {"skirts", "2"},
        {"skirt_height", "1"},
    });

    Model model;
    add_object(model, "cube_extruder_1", 1);
    add_object(model, "cube_extruder_2", 2, {30.0, 0.0, 0.0});

    Print print;
    print.apply(model, config);
    print.validate();
    const std::string gcode{Test::gcode(print)};

    SECTION("Skirt/Brim and Wipe Tower are declared to the host") {
        CHECK(gcode.find("EXCLUDE_OBJECT_DEFINE NAME='Skirt_Brim'") != std::string::npos);
        CHECK(gcode.find("EXCLUDE_OBJECT_DEFINE NAME='Wipe_Tower'") != std::string::npos);
    }

    SECTION("Skirt/Brim and Wipe Tower are never wrapped as excludable") {
        CHECK(gcode.find("EXCLUDE_OBJECT_START NAME='Skirt_Brim'") == std::string::npos);
        CHECK(gcode.find("EXCLUDE_OBJECT_START NAME='Wipe_Tower'") == std::string::npos);
    }
}
