///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ConfigSettingsDialog.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class Navigator;
class LogicalPrinterSettingsDialog;
class ConfigSubcategoryListView;

class PrinterAdvancedSettingsDialog : public ConfigSettingsDialog
{
public:
    explicit PrinterAdvancedSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        LogicalPrinterSettingsDialog* logical_printer_settings_dialog
    );

protected:
    void close_action() override;

private:
    LogicalPrinterSettingsDialog* m_logical_printer_settings_dialog{nullptr};
};

} // namespace Slic3r::App
