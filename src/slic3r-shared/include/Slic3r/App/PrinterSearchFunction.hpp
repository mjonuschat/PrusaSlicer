#pragma once

#include <string>
#include <functional>
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"


namespace Slic3r::App {

int score_printer(std::function<const std::string&(const std::string&)> process_string,
                  const Biz::Preset::PresetItem& item,
                  const std::string& search_text);

} // namespace Slic3r::App
