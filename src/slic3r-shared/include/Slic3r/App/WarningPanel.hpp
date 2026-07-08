///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {
class Text;
class Icon;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class WarningPanel : public Yoga::Rectangle
{
public:
    WarningPanel();

    void set_warning(const std::string& title, const std::string& text = {});
    void set_warning(const std::string& title, const std::vector<std::string>& errors);

private:
    Yoga::Icon* m_warning_icon{nullptr};
    Yoga::Text* m_title{nullptr};
    Yoga::Text* m_text{nullptr};
};

} // namespace Slic3r::App::Plater
