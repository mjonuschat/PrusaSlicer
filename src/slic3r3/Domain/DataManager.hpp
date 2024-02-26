#pragma once
#include <memory>
#include "libslic3r/PrintConfig.hpp"

namespace Slic3r {
class PresetBundle;
}

struct DataManager {
    DataManager();

    std::unique_ptr<Slic3r::PresetBundle> preset_bundle;
    Slic3r::DynamicPrintConfig config;
};




