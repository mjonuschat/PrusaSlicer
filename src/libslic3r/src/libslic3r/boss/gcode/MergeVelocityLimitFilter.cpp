#include "boss/features/merge-velocity-limit/MergeVelocityLimitFeature.hpp"

#include <sstream>

namespace Slic3r::Boss {

namespace {

constexpr std::string_view kAccelPrefix = "SET_VELOCITY_LIMIT ACCEL=";
constexpr std::string_view kScvPrefix = "SET_VELOCITY_LIMIT SQUARE_CORNER_VELOCITY=";

bool starts_with(std::string_view line, std::string_view prefix)
{
    return line.size() >= prefix.size() && line.compare(0, prefix.size(), prefix) == 0;
}

struct ScvLine {
    std::string_view parameter;
    std::string_view comment;
};

ScvLine split_scv_line(std::string_view scv_line)
{
    std::string_view rest = scv_line.substr(std::string_view("SET_VELOCITY_LIMIT ").size());
    while (!rest.empty() && rest.back() == '\r')
        rest.remove_suffix(1);
    if (auto comment_pos = rest.find(" ;"); comment_pos != std::string_view::npos)
        return {rest.substr(0, comment_pos), rest.substr(comment_pos)};
    return {rest, {}};
}

} // namespace

std::string MergeVelocityLimitFeature::filter_layer(std::string gcode)
{
    std::istringstream input(gcode);
    std::ostringstream output;
    std::string line;
    std::string pending_accel_line;

    while (std::getline(input, line)) {
        if (!pending_accel_line.empty()) {
            if (starts_with(line, kScvPrefix)) {
                ScvLine scv = split_scv_line(line);
                auto comment_pos = pending_accel_line.find(" ;");
                if (comment_pos == std::string::npos) {
                    output << pending_accel_line << ' ' << scv.parameter << scv.comment << '\n';
                } else {
                    output << pending_accel_line.substr(0, comment_pos) << ' ' << scv.parameter
                           << pending_accel_line.substr(comment_pos) << '\n';
                }
                pending_accel_line.clear();
                continue;
            }
            output << pending_accel_line << '\n';
            pending_accel_line.clear();
        }

        if (starts_with(line, kAccelPrefix)) {
            pending_accel_line = line;
            continue;
        }
        output << line << '\n';
    }
    if (!pending_accel_line.empty())
        output << pending_accel_line << '\n';

    return output.str();
}

} // namespace Slic3r::Boss
