///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableCategorizer.hpp"
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class Navigator;
class LogicalPrinterSettingsDialog;

class PrinterAdvancedSettingsDialog : public Yoga::AbstractSettingsDialog
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
    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
    LogicalPrinterSettingsDialog* m_logical_printer_settings_dialog{nullptr};
    Biz::ConfigBoxInteractor& m_cbi;

    Biz::UnsharedPointer<ObservableCategorizer> m_observable_categorizer;
    Biz::UnsharedPointer<CategoryPageTransformer> m_category_page_transformer;
};

} // namespace Slic3r::App
