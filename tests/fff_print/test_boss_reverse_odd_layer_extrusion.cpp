#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using Test::TestConfig;
using Test::TestMesh;
using Domain::Percentage;

namespace {

struct XY { double x; double y; };

// Splits gcode at each ";LAYER_CHANGE" marker. Element i is the content of
// the layer whose 0-based id (Layer::id()) is i.
std::vector<std::string> split_layers(const std::string &gcode)
{
    std::vector<std::string> layers;
    std::size_t pos = gcode.find(";LAYER_CHANGE");
    while (pos != std::string::npos) {
        std::size_t next = gcode.find(";LAYER_CHANGE", pos + 1);
        const std::size_t length = next == std::string::npos ? std::string::npos : next - pos;
        layers.push_back(gcode.substr(pos, length));
        pos = next;
    }
    return layers;
}

// Finds the earliest of the given ";TYPE:<role>" markers in a layer's gcode
// and returns the XY of the first G1 move after it -- the start point of
// that role's first extrusion on the layer.
std::optional<XY> first_point_after_role(const std::string &layer_gcode, const std::vector<std::string> &role_tags)
{
    std::size_t marker_pos = std::string::npos;
    for (const std::string &role_tag : role_tags) {
        const std::size_t pos = layer_gcode.find(";TYPE:" + role_tag);
        if (pos != std::string::npos && (marker_pos == std::string::npos || pos < marker_pos)) {
            marker_pos = pos;
        }
    }
    if (marker_pos == std::string::npos) {
        return std::nullopt;
    }

    static const std::regex g1_xy(R"(^G1\s+X([-0-9.]+)\s+Y([-0-9.]+))");
    std::size_t line_start = layer_gcode.find('\n', marker_pos);
    while (line_start != std::string::npos) {
        const std::size_t line_end = layer_gcode.find('\n', line_start + 1);
        const std::size_t length = line_end == std::string::npos ? std::string::npos : line_end - line_start - 1;
        const std::string line = layer_gcode.substr(line_start + 1, length);
        if (std::smatch match; std::regex_search(line, match, g1_xy)) {
            return XY{std::stod(match[1]), std::stod(match[2])};
        }
        line_start = line_end;
    }
    return std::nullopt;
}

TestConfig base_config()
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("support_material").set(Domain::SupportMode::None);
    return config;
}

} // namespace

TEST_CASE("BOSS internal_perimeters_reverse flips the odd layer's perimeter start point", "[boss][gcode][fixture]")
{
    const std::vector<std::string> internal_perimeter{"Perimeter"};

    TestConfig config = base_config();
    config.print.items.opt("perimeters").set(3);
    config.print.items.opt("fill_density").set(Percentage{0});

    config.print.items.opt("internal_perimeters_reverse").set(false);
    const std::vector<std::string> layers_off = split_layers(Test::slice({TestMesh::cube_20x20x20}, config));

    config.print.items.opt("internal_perimeters_reverse").set(true);
    const std::vector<std::string> layers_on = split_layers(Test::slice({TestMesh::cube_20x20x20}, config));

    REQUIRE(layers_off.size() >= 2);
    REQUIRE(layers_on.size() >= 2);

    const std::optional<XY> even_off = first_point_after_role(layers_off[0], internal_perimeter);
    const std::optional<XY> even_on = first_point_after_role(layers_on[0], internal_perimeter);
    const std::optional<XY> odd_off = first_point_after_role(layers_off[1], internal_perimeter);
    const std::optional<XY> odd_on = first_point_after_role(layers_on[1], internal_perimeter);

    REQUIRE(even_off.has_value());
    REQUIRE(even_on.has_value());
    REQUIRE(odd_off.has_value());
    REQUIRE(odd_on.has_value());

    CHECK(even_off->x == even_on->x);
    CHECK(even_off->y == even_on->y);
    CHECK((odd_off->x != odd_on->x || odd_off->y != odd_on->y));
}

TEST_CASE("BOSS overhangs_reverse flips the odd layer's overhang perimeter start point", "[boss][gcode][fixture]")
{
    const std::vector<std::string> perimeter_roles{"Perimeter", "External perimeter", "Overhang perimeter"};

    TestConfig config = base_config();
    // At this layer height, TestMesh::overhang's overhang crosses an
    // odd-numbered layer (Layer::id() % 2 == 1), which this test needs to
    // exercise the odd-layer reversal at all.
    config.print.items.opt("layer_height").set(0.15);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Percentage{0});
    config.print.items.opt("overhangs").set(true);

    config.print.items.opt("overhangs_reverse").set(false);
    const std::vector<std::string> layers_off = split_layers(Test::slice({TestMesh::overhang}, config));

    config.print.items.opt("overhangs_reverse").set(true);
    const std::vector<std::string> layers_on = split_layers(Test::slice({TestMesh::overhang}, config));

    REQUIRE(layers_off.size() == layers_on.size());

    // The overhang only appears partway up the model, so find an odd layer
    // (id() % 2 == 1, i.e. index 1, 3, 5, ...) that actually touches it,
    // rather than assuming layer index 1 does.
    std::optional<std::size_t> odd_overhang_layer;
    for (std::size_t i = 1; i < layers_on.size(); i += 2) {
        if (layers_on[i].find(";TYPE:Overhang perimeter") != std::string::npos) {
            odd_overhang_layer = i;
            break;
        }
    }
    REQUIRE(odd_overhang_layer.has_value());
    const std::size_t layer_index = *odd_overhang_layer;
    REQUIRE(layer_index > 0);

    // Not every layer of this model has perimeter content (some cross-sections
    // are empty), so scan outward from the overhang layer for the nearest even
    // layer that does, rather than assuming its immediate neighbor has one.
    std::optional<std::size_t> even_layer;
    std::size_t best_distance{0};
    for (std::size_t candidate = 0; candidate < layers_on.size(); candidate += 2) {
        if (layers_on[candidate].find(";TYPE:") == std::string::npos) {
            continue;
        }
        const std::size_t distance = candidate > layer_index ? candidate - layer_index : layer_index - candidate;
        if (!even_layer || distance < best_distance) {
            even_layer = candidate;
            best_distance = distance;
        }
    }
    REQUIRE(even_layer.has_value());

    const std::optional<XY> even_off = first_point_after_role(layers_off[*even_layer], perimeter_roles);
    const std::optional<XY> even_on = first_point_after_role(layers_on[*even_layer], perimeter_roles);
    const std::optional<XY> odd_off = first_point_after_role(layers_off[layer_index], perimeter_roles);
    const std::optional<XY> odd_on = first_point_after_role(layers_on[layer_index], perimeter_roles);

    REQUIRE(even_off.has_value());
    REQUIRE(even_on.has_value());
    REQUIRE(odd_off.has_value());
    REQUIRE(odd_on.has_value());

    CHECK(even_off->x == even_on->x);
    CHECK(even_off->y == even_on->y);
    CHECK((odd_off->x != odd_on->x || odd_off->y != odd_on->y));
}

TEST_CASE("BOSS infill_reverse flips the odd layer's infill start point", "[boss][gcode][fixture]")
{
    const std::vector<std::string> internal_infill{"Internal infill"};

    // A pyramid's cross-section shrinks every layer, so the infill polyline set
    // differs layer to layer and sort_fill_extrusions()'s travel-distance chaining
    // is not free to pick the same flip on every layer by coincidence -- unlike a
    // plain cube, where every layer's infill is identical and chaining has no
    // reason to vary. This is the kind of geometry the fix must hold up against.
    TestConfig config = base_config();
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Percentage{20});
    config.print.items.opt("top_solid_layers").set(0);
    config.print.items.opt("bottom_solid_layers").set(0);

    config.print.items.opt("infill_reverse").set(false);
    const std::vector<std::string> layers_off = split_layers(Test::slice({TestMesh::pyramid}, config));

    config.print.items.opt("infill_reverse").set(true);
    const std::vector<std::string> layers_on = split_layers(Test::slice({TestMesh::pyramid}, config));

    REQUIRE(layers_off.size() == layers_on.size());

    // Find two adjacent layers (one even, one odd) that both have internal infill,
    // so the "on" run's odd/even alternation and the "off" run's even-layer
    // baseline can both be read from the same pair of indices.
    std::optional<std::size_t> even_index;
    for (std::size_t i = 0; i + 1 < layers_on.size(); i += 2) {
        if (layers_on[i].find(";TYPE:Internal infill") != std::string::npos
            && layers_on[i + 1].find(";TYPE:Internal infill") != std::string::npos) {
            even_index = i;
            break;
        }
    }
    REQUIRE(even_index.has_value());
    const std::size_t odd_index = *even_index + 1;

    const std::optional<XY> even_off = first_point_after_role(layers_off[*even_index], internal_infill);
    const std::optional<XY> even_on = first_point_after_role(layers_on[*even_index], internal_infill);
    const std::optional<XY> odd_off = first_point_after_role(layers_off[odd_index], internal_infill);
    const std::optional<XY> odd_on = first_point_after_role(layers_on[odd_index], internal_infill);

    REQUIRE(even_off.has_value());
    REQUIRE(even_on.has_value());
    REQUIRE(odd_off.has_value());
    REQUIRE(odd_on.has_value());

    // The option only touches odd layers when it changes something relative to the
    // "off" baseline at the SAME layer index.
    CHECK(even_off->x == even_on->x);
    CHECK(even_off->y == even_on->y);
    CHECK((odd_off->x != odd_on->x || odd_off->y != odd_on->y));

    // The stronger guarantee this option promises: within a single run with the
    // option on, the odd layer's start point actually differs from the even
    // layer's -- true odd/even alternation, not just "different from the off run".
    // A naive XOR of reverse_infill onto sort_fill_extrusions()'s travel-optimized
    // flipped() can fail this even while passing the checks above, if chaining
    // happens to pick opposite natural flips for the two layers.
    CHECK((even_on->x != odd_on->x || even_on->y != odd_on->y));
}
