///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

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
        const Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;

private:
    FnIndexClicked m_on_clicked;
    const Biz::Preset::PresetInteractor& m_preset_interactor;
};

} // namespace Slic3r::App
