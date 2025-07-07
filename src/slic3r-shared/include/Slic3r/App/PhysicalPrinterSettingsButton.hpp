///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/PhysicalPrinter.hpp"

namespace Slic3r::App {

class PhysicalPrinterSettingsButton
    : public Yoga::PrinterSettingsButton,
      public Biz::DataObserver<PhysicalPrinter>
{
public:
    using FnIndexClicked = std::function<void(size_t)>;

    PhysicalPrinterSettingsButton(
        size_t index, const PhysicalPrinter& physical_printer, FnIndexClicked on_clicked
    );

protected:
    void on_data_update() override;

private:
    FnIndexClicked m_on_clicked;
};

} // namespace Slic3r::App
