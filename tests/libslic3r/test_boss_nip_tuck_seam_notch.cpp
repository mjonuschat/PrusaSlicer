#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include "boss/features/nip-tuck-seam/NipTuckSeamFeature.hpp"
#include "boss/foundation/PerimeterGeometryContext.hpp"

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"

#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/GCode/ExtrusionOrder.hpp"
#include "libslic3r/GCode/SmoothPath.hpp"
#include "libslic3r/Geometry/ArcWelder.hpp"
#include "libslic3r/boss/seam/NipTuckSeamNotch.hpp"
#include "libslic3r/libslic3r.h"

using namespace Slic3r;
using namespace Slic3r::Boss;

namespace {

// Minimal single-extruder ConfigView, following FullConfigFDM::defaults()'s
// own fixture shape (this branch has no public helper that builds one, since
// Slic3r::Test::build_fff_printer_config is private to slic3r-domain-tests).
Domain::ConfigView make_config_view(NipTuckSeamType seam_type)
{
    Domain::ConfigPackFDM config_pack;
    config_pack.print.items.opt("seam_type").set(seam_type);

    Domain::Preset::HwPrinterConfig hw_config{
        .technology = Domain::PrinterTechnology::FFF,
        .tool_count = 1,
        .tools      = {Domain::Preset::HwToolConfig{.features = {{"nozzle_diameter", 0.4}}}},
    };

    auto full_config = std::make_shared<Domain::FullConfigFDM>(config_pack, std::vector<unsigned>{0}, hw_config);
    Domain::ConfigView view{full_config, {}};
    view.finalize();
    return view;
}

// A SmoothPath containing the given points as a single open element, in the
// same shape apply_seam_notch expects: front() is the seam start, back() is
// the seam end (the loop's start and end points coincide physically).
GCode::SmoothPath make_smooth_path(const std::vector<Point> &points)
{
    GCode::SmoothPathElement elem;
    for (const Point &p : points) {
        Geometry::ArcWelder::Segment seg;
        seg.point = p;
        seg.radius = 0;
        seg.e_fraction = 1.f;
        seg.height_fraction = 1.f;
        elem.path.push_back(seg);
    }
    GCode::SmoothPath path;
    path.push_back(std::move(elem));
    return path;
}

// Owns the ExtrusionLoop backing a Perimeter fixture's extrusion_entity
// pointer, and the Perimeter itself, so the pointer stays valid for the
// TEST_CASE's lifetime.
struct PerimeterFixture {
    ExtrusionLoop loop;
    GCode::ExtrusionOrder::Perimeter perimeter;
};

std::unique_ptr<PerimeterFixture> make_perimeter_fixture(
    const std::vector<Point> &points, bool external, std::optional<uint16_t> perimeter_index, float width_mm)
{
    auto fixture = std::make_unique<PerimeterFixture>();

    ExtrusionAttributes attributes{
        external ? ExtrusionRole::ExternalPerimeter : ExtrusionRole::Perimeter,
        ExtrusionFlow{0.1, width_mm, 0.2f}};
    if (perimeter_index.has_value())
        attributes.perimeter_index = *perimeter_index;

    Domain::Polyline polyline;
    for (const Point &p : points)
        polyline.points.push_back(p);
    fixture->loop.paths.emplace_back(polyline, attributes);

    fixture->perimeter.smooth_path = make_smooth_path(points);
    fixture->perimeter.reversed = false;
    fixture->perimeter.extrusion_entity = &fixture->loop;
    fixture->perimeter.wipe_offset = 0;

    return fixture;
}

Point mm_point(double x, double y)
{
    return Point(scale_(x), scale_(y));
}

// SmoothPathElement has no operator== (only its .path does, via
// Geometry::ArcWelder::Segment's), so compare geometry element-by-element.
bool smooth_paths_equal(const GCode::SmoothPath &a, const GCode::SmoothPath &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].path != b[i].path)
            return false;
    return true;
}

} // namespace

TEST_CASE("suppress_staggering is false when seam_type is regular", "[boss][seam]")
{
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::Regular);
    CHECK_FALSE(NipTuckSeamFeature::suppress_staggering(config_view, std::optional<int>{1}));
}

TEST_CASE("suppress_staggering is true for perimeter index 1 when seam_type is not regular", "[boss][seam]")
{
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);
    CHECK(NipTuckSeamFeature::suppress_staggering(config_view, std::optional<int>{1}));
}

TEST_CASE("suppress_staggering is false for perimeter index 0 even when seam_type is not regular", "[boss][seam]")
{
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);
    CHECK_FALSE(NipTuckSeamFeature::suppress_staggering(config_view, std::optional<int>{0}));
}

TEST_CASE("suppress_staggering is false when perimeter_index is absent", "[boss][seam]")
{
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);
    CHECK_FALSE(NipTuckSeamFeature::suppress_staggering(config_view, std::nullopt));
}

// A 10mm square, opened at the midpoint of its bottom edge -- a straight
// pass-through at the seam, so the corner-sharpness check doesn't skip it.
std::vector<Point> square_opened_at_edge_midpoint()
{
    return {
        mm_point(5, 0), mm_point(10, 0), mm_point(10, 10), mm_point(0, 10), mm_point(0, 0), mm_point(5, 0)};
}

// The same square, opened at a corner -- the seam's incoming and outgoing
// directions are perpendicular, sharper than the default 44 degree threshold.
std::vector<Point> square_opened_at_corner()
{
    return {mm_point(0, 0), mm_point(10, 0), mm_point(10, 10), mm_point(0, 10), mm_point(0, 0)};
}

// The regular-type gate lives in NipTuckSeamFeature::modify_perimeters (it
// mirrors extrude_perimeters()'s own gate in the ported boss source), not in
// apply_seam_notch itself -- apply_seam_notch_pair applies a notch for any
// seam_type other than Tuck/Nip's own asymmetric skip, Regular included.
TEST_CASE("modify_perimeters does nothing when seam_type is regular", "[boss][seam]")
{
    auto ext = make_perimeter_fixture(square_opened_at_edge_midpoint(), true, uint16_t{0}, 0.42f);
    auto inner = make_perimeter_fixture(square_opened_at_edge_midpoint(), false, uint16_t{1}, 0.42f);
    const GCode::SmoothPath ext_before = ext->perimeter.smooth_path;
    const GCode::SmoothPath inner_before = inner->perimeter.smooth_path;

    std::vector<GCode::ExtrusionOrder::Perimeter> perimeters{ext->perimeter, inner->perimeter};
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::Regular);

    NipTuckSeamFeature::modify_perimeters(PerimeterGeometryContext{perimeters, &config_view, 0});

    CHECK(perimeters.size() == 2);
    CHECK(smooth_paths_equal(perimeters[0].smooth_path, ext_before));
    CHECK(smooth_paths_equal(perimeters[1].smooth_path, inner_before));
}

TEST_CASE("apply_seam_notch skips a corner sharper than seam_notch_angle", "[boss][seam]")
{
    auto ext = make_perimeter_fixture(square_opened_at_corner(), true, uint16_t{0}, 0.42f);
    auto inner = make_perimeter_fixture(square_opened_at_corner(), false, uint16_t{1}, 0.42f);
    const GCode::SmoothPath ext_before = ext->perimeter.smooth_path;
    const GCode::SmoothPath inner_before = inner->perimeter.smooth_path;

    std::vector<GCode::ExtrusionOrder::Perimeter> perimeters{ext->perimeter, inner->perimeter};
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);

    apply_seam_notch(perimeters, config_view, 0);

    CHECK(perimeters.size() == 2);
    CHECK(smooth_paths_equal(perimeters[0].smooth_path, ext_before));
    CHECK(smooth_paths_equal(perimeters[1].smooth_path, inner_before));
}

TEST_CASE("apply_seam_notch applies a V-notch to a straight external perimeter with an inner", "[boss][seam]")
{
    auto ext = make_perimeter_fixture(square_opened_at_edge_midpoint(), true, uint16_t{0}, 0.42f);
    auto inner = make_perimeter_fixture(square_opened_at_edge_midpoint(), false, uint16_t{1}, 0.42f);
    const GCode::SmoothPath ext_before = ext->perimeter.smooth_path;
    const GCode::SmoothPath inner_before = inner->perimeter.smooth_path;

    std::vector<GCode::ExtrusionOrder::Perimeter> perimeters{ext->perimeter, inner->perimeter};
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);

    apply_seam_notch(perimeters, config_view, 0);

    CHECK(perimeters.size() == 2);
    CHECK_FALSE(smooth_paths_equal(perimeters[0].smooth_path, ext_before));
    CHECK_FALSE(smooth_paths_equal(perimeters[1].smooth_path, inner_before));

    // square_opened_at_edge_midpoint() is CCW with reversed = false, seam at
    // (5,0) on the bottom edge, so the square's interior is in +y. A correctly
    // inward notch moves the seam point toward +y, not -y (which would be a
    // visible bump on the outside of the print).
    CHECK(perimeters[0].smooth_path.front().path.front().point.y() >
          ext_before.front().path.front().point.y());
}

TEST_CASE("apply_seam_notch appends a new perimeter when two externals share one inner", "[boss][seam]")
{
    auto ext_close = make_perimeter_fixture(square_opened_at_edge_midpoint(), true, uint16_t{0}, 0.42f);
    auto ext_far = make_perimeter_fixture(
        {mm_point(10, 5), mm_point(20, 5), mm_point(20, 15), mm_point(10, 15), mm_point(10, 5), mm_point(10, 5)},
        true,
        uint16_t{0},
        0.42f);
    auto inner = make_perimeter_fixture(square_opened_at_edge_midpoint(), false, uint16_t{1}, 0.42f);

    std::vector<GCode::ExtrusionOrder::Perimeter> perimeters{ext_close->perimeter, ext_far->perimeter, inner->perimeter};
    const Domain::ConfigView config_view = make_config_view(NipTuckSeamType::NipTuck);

    apply_seam_notch(perimeters, config_view, 0);

    CHECK(perimeters.size() == 4);
}
