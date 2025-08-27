///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/SidebarBed.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
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
    m_physical_printer_settings_dialog(&m_printer_add_dialog),
    m_material_settings_dialog(project_interactor)
{
    set_min_size({240, 60});
    set_orientation(Orientation::Vertical);
    set_gap(10);
    set_flex_shrink(0);

    m_filament_button_group = std::make_shared<ButtonGroup>();
    m_filament_button_group->set_always_checked(false);

    m_bed_name = emplace_back<Text>("Unkown");
    m_bed_name->set_font_type(Render::ImguiFontType::Bold);

    m_physical_printer_button = emplace_back<PrinterSettingsButton>("Physical printer");
    m_physical_printer_button->set_printer_name("NEXT/Elsa");
    m_physical_printer_button->set_preset_name("Prusa NEXT 1T");
    m_physical_printer_button->set_visible(false); // Hide Physical printers for now

    m_logical_printer_button = emplace_back<PrinterSettingsButton>("Logical printer");

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

    m_material_settings_dialog.attach_to_item(this, Position::Left);
    m_material_settings_dialog.callbacks().closed = [this]() {
        for (AbstractButton* button : m_filament_button_group->buttons()) {
            button->set_checked(false);
        }
    };

    m_material_settings_dialog.material_selection_callbacks().advanced_settings_tab_opened =
        [this](size_t current_index) {
        if (m_material_settings_dialog.opened()) {
            dynamic_cast<AbstractButton*>(m_list_view->get_item(current_index))->set_checked(true);
        }
    };

    m_filament_button_group->callbacks().checked_changed =
        [this](AbstractButton* current_check, AbstractButton* last_check) {
        if (current_check) {
            m_material_settings_dialog.open();
            m_material_settings_dialog.set_material_index(m_list_view->index_of(current_check).value());
        } else {
            m_material_settings_dialog.close();
        }
    };

    m_project_interactor.preset_interactor()
        .printer_presets()
        .add_listener<Biz::IListSelectionChangedListener>(this);
    on_list_selection_changed(
        m_project_interactor.preset_interactor().printer_presets().selected_index()
    );

    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        this
    );
    on_selected_bed_instances_changed(
        m_project_interactor.selected_project_id(),
        m_project_interactor.scene_interactor().bed_selection()
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

    const std::string prefix{preset_item.runtime_only ? _u8L("(From 3mf) ") : ""};
    m_logical_printer_button->set_printer_name(prefix + preset_item.name);
    m_logical_printer_button->set_preset_name(preset_item.hw_printer_config_name);

    const Domain::Preset::EvaluatedPrinterPreset& printer_preset = m_project_interactor
                                                                       .preset_interactor()
                                                                       .current_printer_preset();

    if (printer_preset.hw_config.visual.thumbnail.has_value()) {
        const std::string image_path = printer_preset.hw_config.relative_path_to_assets()
            + printer_preset.hw_config.visual.thumbnail.value();

        m_logical_printer_button->set_image(image_path);
    }
}

void SidebarBed::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::BedSelection& bed_selection
)
{
    if (project_id == Domain::INVALID_ID || m_project_interactor.selected_project_id() != project_id)
    {
        return;
    }

    Domain::BedInstance* bed_instance = m_project_interactor.selected_project().find_bed_instance_by_id(
        bed_selection.last_selected_bed().instance_id
    );

    ASSERT(bed_instance);

    m_bed_name->set_text(bed_instance->name());
}

} // namespace Slic3r::App
