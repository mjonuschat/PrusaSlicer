///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

namespace Slic3r::App {


class FilamentSettingsDialog : public Yoga::AbstractSettingsDialog {
public:
    FilamentSettingsDialog();


private:
    Biz::ObservableList<PageEntry> m_list_pages;
};

}
