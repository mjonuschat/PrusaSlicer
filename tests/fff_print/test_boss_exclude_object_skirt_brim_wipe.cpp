#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Slicing/BackgroundProcess.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "libslic3r/Print.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using Test::TestConfig;
using Domain::VolumeSettings;
using Biz::Algorithms::ModelObject::add_volume;
using Biz::Algorithms::ModelObject::ensure_on_bed;
using Biz::Algorithms::ModelVolume::translate;

namespace {

void add_object(
    Domain::Model &model, const std::string &name, const int extruder, const Vec3d &offset = Vec3d::Zero()
)
{
    Domain::ModelObject *object = model.add_object();
    object->name = name;
    Domain::ModelVolume *volume = add_volume(object, Test::mesh(Test::TestMesh::cube_20x20x20));
    translate(*volume, offset);

    VolumeSettings volume_settings;
    volume_settings.overrides.set("extruder", extruder);
    volume->volume_settings = volume_settings;

    object->add_instance();
    ensure_on_bed(*object);
}

std::string slice_model(Domain::Model &model, const TestConfig &config)
{
    Domain::Bed model_bed;
    Domain::BedInstance bed_instance{model_bed};
    for (const Domain::ModelObject *object : model.objects)
        for (Domain::ModelInstance *instance : object->instances)
            bed_instance.model_instances.push_back(instance);

    Print print;
    auto preset_metadata = Test::create_dummy_selected_preset_metadata(Test::create_dummy_hw_config(2));
    auto metadata = Biz::Slicing::build_gcode_metadata({}, preset_metadata, config);
    print.update(
        model,
        config,
        bed_instance,
        preset_metadata,
        Biz::Slicing::build_metadata_serializer(metadata, preset_metadata, config)
    );
    print.validate();
    return Test::gcode(print);
}

} // namespace

TEST_CASE(
    "Skirt and wipe tower are declared but never wrapped as excludable under Klipper firmware labeling",
    "[LabelObjects]"
)
{
    TestConfig config{2};
    config.printer.items.opt("gcode_flavor").set(Domain::GCodeFlavor::gcfKlipper);
    config.print.items.opt("gcode_label_objects").set(Domain::LabelObjectsStyle::Firmware);
    config.print.items.opt("gcode_comments").set(true);
    config.print.items.opt("wipe_tower").set(true);
    config.print.items.opt("skirts").set(2);
    config.print.items.opt("skirt_height").set(1);

    Domain::Model model;
    add_object(model, "cube_extruder_1", 1);
    add_object(model, "cube_extruder_2", 2, {30.0, 0.0, 0.0});

    const std::string gcode{slice_model(model, config)};

    SECTION("Skirt/Brim and Wipe Tower are declared to the host") {
        CHECK(gcode.find("EXCLUDE_OBJECT_DEFINE NAME='Skirt_Brim'") != std::string::npos);
        CHECK(gcode.find("EXCLUDE_OBJECT_DEFINE NAME='Wipe_Tower'") != std::string::npos);
    }

    SECTION("Skirt/Brim and Wipe Tower are never wrapped as excludable") {
        CHECK(gcode.find("EXCLUDE_OBJECT_START NAME='Skirt_Brim'") == std::string::npos);
        CHECK(gcode.find("EXCLUDE_OBJECT_START NAME='Wipe_Tower'") == std::string::npos);
    }
}
