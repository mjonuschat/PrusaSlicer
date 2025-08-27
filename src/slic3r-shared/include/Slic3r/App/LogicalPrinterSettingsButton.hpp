///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::Domain {
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::App {

class LogicalPrinterSettingsButton :
    public Yoga::PrinterSettingsButton,
    public Biz::DataObserver<Biz::Preset::PresetItem>
{
public:
    using FnIndexClicked = std::function<void(size_t)>;

    LogicalPrinterSettingsButton(
        size_t index,
        const Biz::Preset::PresetItem& logical_printer_preset,
        FnIndexClicked on_clicked,
        const Domain::Workbench& workbench
    );

protected:
    void on_data_update() override;

private:
    FnIndexClicked m_on_clicked;
    const Domain::Workbench& m_workbench;
};

} // namespace Slic3r::App
