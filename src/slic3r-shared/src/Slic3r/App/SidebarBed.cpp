///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/SidebarBed.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <imgui/imgui_internal.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

SidebarBed::SidebarBed(Biz::ProjectInteractor& project_interactor) :
    Window("sidebar_bed"),
    m_project_interactor(project_interactor),
    m_printer_settings_dialog(project_interactor, &m_printer_add_dialog),
    m_filament_settings_dialog(project_interactor),
    m_physical_printer_settings_dialog(&m_printer_add_dialog)
{
    set_min_size({240, 60});
    set_orientation(Orientation::Vertical);
    set_gap(10);
    set_flex_shrink(0);

    m_filament_button_group = std::make_shared<ButtonGroup>();

    m_bed_name = emplace_back<Text>("Bed 1");
    m_bed_name->set_font_type(Render::ImguiFontType::Bold);

    m_physical_printer_button = emplace_back<PrinterSettingsButton>("Physical printer");
    m_physical_printer_button->set_icon(Render::Icon::PrinterNEXT);
    m_physical_printer_button->set_printer_name("NEXT/Elsa");
    m_physical_printer_button->set_preset_name("Prusa NEXT 1T");

    m_logical_printer_button = emplace_back<PrinterSettingsButton>("Logical printer");
    m_logical_printer_button->set_icon(Render::Icon::PrinterNEXT);

    m_printer_settings_dialog.attach_to_item(this, Position::Left);
    m_printer_settings_dialog.callbacks().closed = [this]() {
        m_logical_printer_button->set_checked(false);
    };

    m_physical_printer_settings_dialog.attach_to_item(this, Position::Left);
    m_physical_printer_settings_dialog.callbacks().closed = [this]() {
        m_physical_printer_button->set_checked(false);
    };

    m_logical_printer_button->callbacks().checked_changed = [this](bool checked) {
        if (checked) {
            m_printer_settings_dialog.open();
            m_physical_printer_settings_dialog.close();
        } else {
            m_printer_settings_dialog.close();
        }
    };

    m_physical_printer_button->callbacks().checked_changed = [this](bool checked) {
        if (checked) {
            m_physical_printer_settings_dialog.open();
            m_printer_settings_dialog.close();
        } else {
            m_physical_printer_settings_dialog.close();
        }
    };

    m_logical_printer_button->on_cog() = []() {
        // ToDo open some other settings dialog
    };

    m_list_view = emplace_back<MaterialListView>(std::weak_ptr<ButtonGroup>(m_filament_button_group));
    m_list_view->set_source_list(&m_project_interactor.preset_interactor().material_presets());
    m_list_view->set_orientation(Orientation::Vertical);
    m_list_view->set_gap(5);

    m_filament_settings_dialog.attach_to_item(this, Position::Left);
    m_filament_settings_dialog.callbacks().closed = [this]() {
        for (AbstractButton* button : m_filament_button_group->buttons()) {
            button->set_checked(false);
        }
    };

    m_filament_settings_dialog.dialog_callbacks().tab_selected = [this](size_t current_index) {
        dynamic_cast<AbstractButton*>(m_list_view->get_item(current_index))->set_checked(true);
    };

    m_filament_button_group->callbacks().checked_changed =
        [this](AbstractButton* current_check, AbstractButton* last_check) {
        if (current_check) {
            m_filament_settings_dialog.open();
            m_filament_settings_dialog.set_current_tab(m_list_view->index_of(current_check).value());
        } else {
            m_filament_settings_dialog.close();
        }
    };

    m_project_interactor.preset_interactor()
        .printer_presets()
        .add_listener<Biz::IListSelectionChangedListener>(this);
    on_list_selection_changed(
        m_project_interactor.preset_interactor().printer_presets().selected_index()
    );
}

void SidebarBed::on_list_selection_changed(Domain::SelectionId new_selection)
{
    if (new_selection == Domain::INVALID_ID) {
        return;
    }

    const Biz::Preset::PresetItem& preset_item = m_project_interactor.preset_interactor()
                                                     .printer_presets()
                                                     .items()
                                                     .at(new_selection);

    m_logical_printer_button->set_printer_name(preset_item.name);
    m_logical_printer_button->set_preset_name(preset_item.hw_pritner_config_name);
}

} // namespace Slic3r::App
