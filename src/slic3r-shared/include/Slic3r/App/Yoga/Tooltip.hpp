///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AttachedWindow.hpp"

namespace Slic3r::App::Yoga {

class Text;

class Tooltip : public AttachedWindow
{
public:
    Tooltip(
        const std::string& window_name,
        const std::string& text,
        const std::string& shortcut
    );

    const std::string& text() const;
    void set_text(const std::string& text);

    const std::string& shortcut() const;
    void set_shortcut(const std::string& shortcut);

private:
    Text* m_text = nullptr;
    Text* m_shortcut = nullptr;
};

} // namespace Slic3r::App::Yoga
