// The native "Classic" jitter draws from std::random_device (real hardware
// entropy on this platform), so Classic-vs-structured output while fuzzy
// skin is on cannot be compared byte-for-byte across process runs. The
// second test case below instead proves the deterministic half: with fuzzy
// skin disabled, fuzzy_skin_noise_type is never read, so it cannot affect
// the emitted G-code either way.
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Slicing/BackgroundProcess.hpp"
#include "boss/features/structured-fuzzy-skin/StructuredFuzzySkinFeature.hpp"
#include "libslic3r/Print.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using Test::TestConfig;
using Biz::Algorithms::ModelObject::add_volume;
using Biz::Algorithms::ModelObject::ensure_on_bed;

namespace {

Domain::Model build_model()
{
    Domain::Model model;
    Domain::ModelObject *object = model.add_object();
    object->name = "cube";
    add_volume(object, Test::mesh(Test::TestMesh::cube_20x20x20));
    object->add_instance();
    ensure_on_bed(*object);
    return model;
}

std::string slice_model(Domain::Model &model, const TestConfig &config)
{
    Domain::Bed model_bed;
    Domain::BedInstance bed_instance{model_bed};
    for (const Domain::ModelObject *object : model.objects)
        for (Domain::ModelInstance *instance : object->instances)
            bed_instance.model_instances.push_back(instance);

    Print print;
    auto preset_metadata = Test::create_dummy_selected_preset_metadata(Test::create_dummy_hw_config(1));
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

// Drops the trailing "; prusaslicer_config" comment block, which echoes
// every option's value regardless of whether it affects output.
std::string strip_comments(const std::string &gcode)
{
    std::istringstream input{gcode};
    std::ostringstream output;
    std::string        line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] != ';')
            output << line << '\n';
    }
    return output.str();
}

TestConfig make_config()
{
    TestConfig config{1};
    config.print.items.opt("fuzzy_skin").set(Domain::FuzzySkinType::External);
    config.print.items.opt("fuzzy_skin_thickness").set(0.3);
    config.print.items.opt("fuzzy_skin_point_dist").set(0.5);
    return config;
}

} // namespace

TEST_CASE("A structured noise type actually perturbs the perimeter path", "[boss][surface][fixture]")
{
    Domain::Model unfuzzed_model = build_model();
    TestConfig    unfuzzed_config{1};
    const std::string baseline_gcode = slice_model(unfuzzed_model, unfuzzed_config);

    Domain::Model fuzzed_model = build_model();
    TestConfig    fuzzed_config = make_config();
    fuzzed_config.print.items.opt("fuzzy_skin_noise_type").set(Domain::Boss::FuzzySkinNoiseType::Perlin);
    fuzzed_config.print.items.opt("fuzzy_skin_feature_size").set(2.0);
    fuzzed_config.print.items.opt("fuzzy_skin_octaves").set(3);
    fuzzed_config.print.items.opt("fuzzy_skin_persistence").set(0.5);
    const std::string fuzzed_gcode = slice_model(fuzzed_model, fuzzed_config);

    CHECK(strip_comments(fuzzed_gcode) != strip_comments(baseline_gcode));
}

TEST_CASE("Classic noise type still fuzzifies the perimeter when fuzzy skin is enabled", "[boss][surface][fixture]")
{
    // Exercises the enabled-Classic path (unlike the disabled-path test
    // below), so get_displacement()'s pa point-math actually runs here.
    // See test_boss_fuzzy_skin_classic_rounding.cpp for the numeric bound
    // on that math, since real hardware entropy in random_value() rules
    // out a byte-identity check against the pre-feature formula here.
    Domain::Model unfuzzed_model = build_model();
    TestConfig    unfuzzed_config{1};
    const std::string baseline_gcode = slice_model(unfuzzed_model, unfuzzed_config);

    Domain::Model classic_model = build_model();
    TestConfig    classic_config = make_config();
    classic_config.print.items.opt("fuzzy_skin_noise_type").set(Domain::Boss::FuzzySkinNoiseType::Classic);
    const std::string classic_gcode = slice_model(classic_model, classic_config);

    CHECK(strip_comments(classic_gcode) != strip_comments(baseline_gcode));
}

TEST_CASE("fuzzy_skin_noise_type is inert while fuzzy skin itself is disabled", "[boss][surface][fixture]")
{
    Domain::Model classic_model = build_model();
    TestConfig    classic_config{1};
    classic_config.print.items.opt("fuzzy_skin_noise_type").set(Domain::Boss::FuzzySkinNoiseType::Classic);
    const std::string classic_gcode = slice_model(classic_model, classic_config);

    Domain::Model perlin_model = build_model();
    TestConfig    perlin_config{1};
    perlin_config.print.items.opt("fuzzy_skin_noise_type").set(Domain::Boss::FuzzySkinNoiseType::Perlin);
    perlin_config.print.items.opt("fuzzy_skin_feature_size").set(5.0);
    perlin_config.print.items.opt("fuzzy_skin_octaves").set(8);
    perlin_config.print.items.opt("fuzzy_skin_persistence").set(0.9);
    const std::string perlin_gcode = slice_model(perlin_model, perlin_config);

    // fuzzy_skin defaults to None on both configs, so should_fuzzify() never reads fuzzy_skin_noise_type.
    CHECK(strip_comments(classic_gcode) == strip_comments(perlin_gcode));
}
