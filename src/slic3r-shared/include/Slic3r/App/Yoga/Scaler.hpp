///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Scaler : public Item
{
public:
    void style_node() override;

private:
    float get_size() const;
    void set_size(float size);

private:
    float m_size = YGUndefined;
};

} // namespace Slic3r::App::Yoga
