///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/PhysicalPrinterSettingsButton.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterConfig.hpp"
#include "Slic3r/Biz/PhysicalPrinter/IPhysicalPrinterChangedListener.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"
#include "Slic3r/App/ConfigSettingsDialog.hpp"
#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::PhysicalPrinter {
class PhysicalPrinterInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class StackLayout;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class Navigator;

class PrinterAddDialog;
class PhysicalPrinterAdvancedSettingsDialog;

class PhysicalPrinterSettingsDialog :
    public Yoga::Dialog,
    public Biz::PhysicalPrinter::IPhysicalPrinterChangedListener,
    public Biz::Preset::IPresetChangedListener
{
public:
    PhysicalPrinterSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        PrinterAddDialog* printer_add_dialog,
        Navigator& navigator
    );

    void on_printer_data_changed() override;
    void on_selected_physical_printer_changed() override;

    PhysicalPrinterAdvancedSettingsDialog& print_host_settings_dialog();

    void on_preset_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::PresetItemType type
    ) override;

protected:
    void close_action() override;

private:
    void create_page_list();

    void create_page_settings();

    void check_printer_button(size_t index);

    void on_about_to_show() override;

    void filter_printer_buttons();

private:
    using PrinterListView = Yoga::ListView<
        PhysicalPrinterSettingsButton,
        Biz::PhysicalPrinter::PhysicalPrinterConfig,
        Yoga::ViewFactory<
            PhysicalPrinterSettingsButton,
            Biz::PhysicalPrinter::PhysicalPrinterConfig,
            PhysicalPrinterSettingsButton::FnIndexClicked>>;

    Biz::ListenerScope<
        Biz::PhysicalPrinter::IPhysicalPrinterChangedListener,
        Biz::PhysicalPrinter::PhysicalPrinterInteractor,
        PhysicalPrinterSettingsDialog>
        m_physical_printer_changed_listener_scope;

    Biz::ListenerScope<
        Biz::Preset::IPresetChangedListener,
        Biz::Preset::PresetInteractor,
        PhysicalPrinterSettingsDialog>
        m_preset_changed_listener_scope;

    PrinterAddDialog* m_printer_add_dialog{nullptr};
    Biz::ProjectInteractor& m_project_interactor;
    Biz::PhysicalPrinter::PhysicalPrinterInteractor& m_physical_printer_interactor;
    Navigator& m_navigator;

    //Biz::UnsharedPointer<Biz::ObservableList<Biz::PhysicalPrinter::PhysicalPrinterConfig>> m_list_physical_printers;
    PrinterListView* m_printer_list_view{nullptr};
    PhysicalPrinterAdvancedSettingsDialog* m_print_host_settings_dialog{nullptr};
    Yoga::StackLayout* m_stack_layout{nullptr};
    Yoga::Item* m_page_list{nullptr};
    Yoga::Item* m_page_settings{nullptr};
    Yoga::ButtonGroup m_group_keywords;
    Yoga::Text* m_text_printer_name{nullptr};
    Yoga::Text* m_text_host_type_name{nullptr};
    Yoga::LayoutButton* m_button_advanced_setting{nullptr};
    Yoga::LayoutButton* m_button_delete{nullptr};
};
}