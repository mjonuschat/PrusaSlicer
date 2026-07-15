///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

#include "Slic3r/App/ConfigSettingsDialog.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Text;
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;
class LogicalPrinterSettingsDialog;

class PrinterAdvancedSettingsDialog :
    public ConfigSettingsDialog,
    public Biz::IListSelectionChangedListener,
    public Biz::Preset::IPresetChangedListener
{
public:
    explicit PrinterAdvancedSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        LogicalPrinterSettingsDialog* logical_printer_settings_dialog
    );

    void on_list_selection_changed(Domain::SelectionId new_selection) override;

    void on_preset_value_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        const Domain::ConfigItem& item
    ) override;

protected:
    void close_action() override;

private:
    Biz::ListenerScope<
        Biz::IListSelectionChangedListener,
        Biz::Preset::PresetItemObservableList,
        PrinterAdvancedSettingsDialog>
        m_list_selection_changed_scope;

    Biz::ListenerScope<
        Biz::Preset::IPresetChangedListener,
        Biz::Preset::PresetInteractor,
        PrinterAdvancedSettingsDialog>
        m_preset_changed_listener_scope;

    LogicalPrinterSettingsDialog* m_logical_printer_settings_dialog{nullptr};

    Yoga::Text* m_label_preset_name{nullptr};
    Yoga::LayoutButton* m_revert_button{nullptr};
};

} // namespace Slic3r::App
