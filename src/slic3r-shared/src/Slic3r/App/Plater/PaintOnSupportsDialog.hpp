///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"

#include <memory>

namespace Slic3r::App::Plater {

class PaintOnSupportsDialog : public Yoga::Dialog
{
public:
    PaintOnSupportsDialog();

private:
    void add_new_row(const std::string& title, std::unique_ptr<Item> controls);
    Item* add_helper(const std::vector<wchar_t> symbols, const std::string title, Item* help_row);
};

} // namespace Slic3r::App::Plater
