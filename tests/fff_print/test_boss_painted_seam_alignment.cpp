#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <libslic3r/Point.hpp>
#include <libslic3r/GCode/SeamAligned.hpp>
#include <libslic3r/GCode/SeamChoice.hpp>

using namespace Slic3r;
using namespace Slic3r::Seams;
using namespace Catch;

// No pre-painted 3MF fixture exercises an aligned-seam enforcer region
// (tests/data/seam_test_object.3mf, used by test_seam_aligned.cpp, carries no
// seam painting), and no test helper paints a mesh region programmatically.
// Perimeters are built by hand instead, mirroring the pattern already used
// by test_seam_aligned.cpp's AlignedTest::get_perimeter().
//
// The enforcer region drifts along X by 0.03 per layer so forward blending
// (tracking a moving target) and backward smoothing (pulling the shell
// start forward) both have something to do. Y stays fixed at 0.5, so the
// clustering check below stays independent of the drift.
namespace PaintedAlignmentTest {

constexpr double drift_per_layer{0.03};

Perimeters::Perimeter make_perimeter(const double slice_z, const std::size_t layer_index) {
    const double drift_x{drift_per_layer * static_cast<double>(layer_index)};
    std::vector<Vec2d> positions{
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {drift_x, 0.55}, {drift_x, 0.45}};
    std::vector<double> angles(positions.size(), -M_PI / 2.0);
    std::vector<Perimeters::PointType> point_types(positions.size(), Perimeters::PointType::common);
    point_types[4] = Perimeters::PointType::enforcer;
    point_types[5] = Perimeters::PointType::enforcer;
    std::vector<Perimeters::PointClassification>
        point_classifications(positions.size(), Perimeters::PointClassification::common);
    std::vector<Perimeters::AngleType> angle_types(positions.size(), Perimeters::AngleType::concave);
    angle_types[4] = Perimeters::AngleType::smooth;
    angle_types[5] = Perimeters::AngleType::smooth;

    return {
        slice_z,
        layer_index,
        false,
        std::move(positions),
        std::move(angles),
        std::move(point_types),
        std::move(point_classifications),
        std::move(angle_types)};
}

Shells::Shell<> make_painted_shell(const std::size_t layer_count) {
    Shells::Shell<> shell;
    for (std::size_t i{0}; i < layer_count; ++i) {
        shell.push_back({make_perimeter(static_cast<double>(i) * 0.1, i), i});
    }
    return shell;
}

} // namespace PaintedAlignmentTest

TEST_CASE(
    "Painted seam alignment clusters the enforcer region, lags a drifting "
    "target via forward blending, and pulls the shell start forward via "
    "backward smoothing",
    "[Seams][boss][seam][Integration]"
) {
    using namespace PaintedAlignmentTest;

    constexpr std::size_t layer_count{8};
    Shells::Shells<> shells;
    shells.push_back(make_painted_shell(layer_count));

    Aligned::Params params{};
    params.max_detour = 0.4;
    params.jump_visibility_threshold = 1e6;
    params.continuity_modifier = 0.0;

    const Aligned::SeamChoiceVisibility visibility_calculator{
        [](const SeamChoice &, const Perimeters::Perimeter &) { return 0.0; }};

    const std::vector<std::vector<SeamPerimeterChoice>> layer_seams{
        Aligned::get_object_seams(std::move(shells), visibility_calculator, params)};

    REQUIRE(layer_seams.size() == layer_count);

    std::vector<Vec2d> positions;
    for (const std::vector<SeamPerimeterChoice> &layer : layer_seams) {
        REQUIRE(layer.size() == 1);
        positions.push_back(layer.front().choice.position);
    }

    // Clustering: the enforcer pair straddles y = 0.5 on every layer. A
    // naive nearest-single-vertex snap would land 0.05 away on Y; only
    // merging them via cluster_positions()/get_enforcer_centroid_near()
    // lands this close.
    constexpr double centroid_tolerance{0.02};
    std::size_t at_centroid{0};
    for (const Vec2d &position : positions) {
        if (std::abs(position.y() - 0.5) <= centroid_tolerance) {
            ++at_centroid;
        }
    }
    CHECK(at_centroid >= layer_count - 1);

    // Forward blending: the raw per-layer drift is 0.03. A seam that snapped
    // straight to each layer's target would move by that much every step;
    // the blend must lag behind it instead, so each step stays well short
    // of 0.03 while still moving forward.
    for (std::size_t i{1}; i < positions.size(); ++i) {
        const double step{(positions[i] - positions[i - 1]).norm()};
        CHECK(step > 1e-3);
        CHECK(step < 0.02);
    }

    // Backward smoothing: layer 0 carries no drift, so a forward-only pass
    // would leave it exactly at x == 0. Backward smoothing pulls it toward
    // the later, drifted layers instead.
    CHECK(positions.front().x() > 1e-3);
    CHECK(positions.front().x() > 0.25 * positions[3].x());
}
