#include <boost/lexical_cast.hpp>
#include <regex>
#include <vector>

#include <libslic3r/Point.hpp>
#include <libslic3r/BoundingBox.hpp>
#include <libslic3r/Model.hpp>

namespace {
struct Extrusion {
    Slic3r::Vec4d start;
    Slic3r::Vec4d end;
};

std::vector<Extrusion> get_first_layer_extrusions(std::istream& gcode) {
    Slic3r::Vec4d previous_point{Slic3r::Vec4d::Zero()};

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

        Slic3r::Vec4d current_point{previous_point};
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

bool is_within(const Extrusion &extrusion, const Slic3r::BoundingBoxf &bounding_box) {
    if (!bounding_box.contains(extrusion.start.head<2>())) {
        return false;
    }
    if (!bounding_box.contains(extrusion.end.head<2>())) {
        return false;
    }
    return true;
}

std::optional<std::string> are_extrusions_within_bounds(
    const std::vector<Extrusion>& extrusions,
    const std::vector<Slic3r::BoundingBoxf>& bounding_boxes
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
            const Slic3r::BoundingBoxf& bounding_box{bounding_boxes[i]};
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

std::optional<std::string> is_gcode_sane(const std::string& gcode, const Slic3r::Model &model) {
    if (gcode.empty()) {
        return "GCode is empty!";
    }

    std::stringstream buffer{gcode};
    const std::vector<Extrusion> extrusions{get_first_layer_extrusions(buffer)};
    std::vector<Slic3r::BoundingBoxf> bounding_boxes;
    for (const Slic3r::ModelObject* object : model.objects) {
        bounding_boxes.push_back(Slic3r::BoundingBoxf{
            object->bounding_box_exact().min.head<2>(),
            object->bounding_box_exact().max.head<2>()
        });
    }

    return are_extrusions_within_bounds(extrusions, bounding_boxes);
}

}
