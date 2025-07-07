///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

namespace Slic3r::App {

class PrinterAdvancedSettingsDialog : public Yoga::AbstractSettingsDialog
{
public:
    PrinterAdvancedSettingsDialog();

private:
    Biz::ObservableList<PageEntry> m_list_pages;
};

} // namespace Slic3r::App
