#include <boost/lexical_cast.hpp>
#include <regex>
#include <vector>

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Model.hpp"

using namespace Slic3r;
using namespace Slic3r::Biz;

namespace {
struct Extrusion {
    Domain::Vec4d start;
    Domain::Vec4d end;
};

std::vector<Extrusion> get_first_layer_extrusions(std::istream& gcode) {
    Domain::Vec4d previous_point{Domain::Vec4d::Zero()};

    std::vector<Extrusion> result;

    std::string line;
    while (std::getline(gcode, line)) {
        if (!line.starts_with("G1")) {
            continue;
        }
        const std::regex pattern(R"(\b([XYZE])([-+]?\d*\.?\d+))");

        const auto begin{std::sregex_iterator(line.begin(), line.end(), pattern)};
        const auto end{std::sregex_iterator()};
        if (begin == end) {
            continue;
        }

        Domain::Vec4d current_point{previous_point};
        for (auto it{begin}; it != end; ++it) {
            std::string value{(*it)[2]};
            if ((*it)[1] == "X") {
                current_point[0] = boost::lexical_cast<double>(value);
            }
            if ((*it)[1] == "Y") {
                current_point[1] = boost::lexical_cast<double>(value);
            }
            if ((*it)[1] == "Z") {
                current_point[2] = boost::lexical_cast<double>(value);
            }
            if ((*it)[1] == "E") {
                current_point[3] = boost::lexical_cast<double>(value);
            }
        }

        if (current_point.z() > previous_point.z() && previous_point.z() != 0) {
            return result;
        }

        const auto epsilon{std::numeric_limits<double>::epsilon()};
        if (
            (current_point.head<3>() - previous_point.head<3>()).norm() > epsilon
            && current_point[3] - previous_point[3] > 0
        ) {
            result.push_back({previous_point, current_point});
        }
        previous_point = current_point;
    }
    throw std::runtime_error{"Did not find the end of the first layer!"};
}

bool is_within(const Extrusion &extrusion, const Domain::BoundingBox2d &bounding_box) {
    if (!Algorithms::BoundingBox::contains(bounding_box, extrusion.start.head<2>())) {
        return false;
    }
    if (!Algorithms::BoundingBox::contains(bounding_box, extrusion.end.head<2>())) {
        return false;
    }
    return true;
}

std::optional<std::string> are_extrusions_within_bounds(
    const std::vector<Extrusion>& extrusions,
    const std::vector<Domain::BoundingBox2d>& bounding_boxes
) {
    if (bounding_boxes.empty()) {
        return "No bounding boxes were provided. There might be a problem with the model.";
    }
    if (extrusions.empty()) {
        return "There are no extrusions in the gcode!";
    }
    std::vector<bool> has_extrusions(bounding_boxes.size(), false);

    for (const Extrusion &extrusion : extrusions) {
        bool is_within_one{false};
        for (std::size_t i{}; i < bounding_boxes.size(); ++i) {
            const Domain::BoundingBox2d& bounding_box{bounding_boxes[i]};
            if (is_within(extrusion, bounding_box)) {
                is_within_one = true;
                has_extrusions[i] = true;
                break;
            }
        }
        if (!is_within_one) {
            return "There is an extrusion outside all objects bounding boxes.";
        }
    }

    if (std::ranges::any_of(has_extrusions, [](const bool has){return !has;})) {
        return "Ther is an object with no extrusions within its bounding box.";
    }

    return std::nullopt;
}
}

namespace Slic3r::Tests {

std::optional<std::string> is_gcode_sane(const std::string& gcode, const Domain::Model &model) {
    if (gcode.empty()) {
        return "GCode is empty!";
    }

    std::stringstream buffer{gcode};
    const std::vector<Extrusion> extrusions{get_first_layer_extrusions(buffer)};
    std::vector<Domain::BoundingBox2d> bounding_boxes;
    for (const Domain::ModelObject* object : model.objects) {
        bounding_boxes.push_back(Domain::BoundingBox2d{
            Algorithms::ModelObject::bounding_box_exact(*object).min.head<2>(),
            Algorithms::ModelObject::bounding_box_exact(*object).max.head<2>()
        });
    }

    return are_extrusions_within_bounds(extrusions, bounding_boxes);
}

}
