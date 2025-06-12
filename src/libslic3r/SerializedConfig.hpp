#pragma once

#include <string>

namespace Slic3r::Biz::Print {
struct SerializedConfig {
    std::string json;
    std::string ini;
};
}

