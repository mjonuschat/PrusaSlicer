#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

class IconButton : public AbstractButton {
public:

    explicit IconButton(wchar_t icon, const std::string& tooltip = {}, Item* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;
};

}
