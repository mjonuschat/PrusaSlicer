///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/Biz/ObservableList.hpp"

namespace Slic3r::App {

class PrintSettingsDialog : public Yoga::AbstractSettingsDialog
{
public:
    PrintSettingsDialog();

protected:
    void on_tab_selected(int current_index) override;

private:
    Biz::ObservableList<PageEntry> m_list_pages_print;
    Biz::ObservableList<PageEntry> m_list_pages_tool;
};

} // namespace Slic3r::App
