///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include "Slic3r/App/PrinterAddDialog.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;

Slic3r::App::PrinterSettingsDialog::PrinterSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    PrinterAddDialog* printer_add_dialog
) :
    Dialog({"Printers"}, "PrinterSettingsDialog"),
    m_preset_changed_listener_scope(project_interactor.preset_interactor(), *this),
    m_project_interactor(project_interactor),
    m_advanced_dialog(project_interactor),
    m_printer_add_dialog(printer_add_dialog)
{
    content_item()->set_width(350);

    content()->set_padding(0);
    content()->set_orientation(Orientation::Vertical);

    m_stack_layout = content()->emplace_back<StackLayout>();
    m_stack_layout->set_orientation(Orientation::Vertical);

    create_page_list();

    create_page_settings();

    m_stack_layout->set_current_index(0);
}

void Slic3r::App::PrinterSettingsDialog::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id
        && type == Biz::Preset::PresetItemType::PrinterPreset)
    {
        const Domain::Preset::HwPrinterConfig& printer_config = m_project_interactor.preset_interactor().current_printer_config();

        m_text_printer_name->set_text(printer_config.name);

        if (printer_config.visual.thumbnail.has_value()) {
            const std::string image_path = printer_config.relative_path_to_assets()
                + printer_config.visual.thumbnail.value();

            m_printer_icon->set_image(image_path);
        }
    }
}

void Slic3r::App::PrinterSettingsDialog::create_page_list()
{
    m_page_list = m_stack_layout->emplace_back<Item>();
    m_page_list->set_orientation(Orientation::Vertical);
    m_page_list->set_gap(5);
    m_page_list->set_padding(10);

    Item* keywords_row = m_page_list->emplace_back<Item>();
    keywords_row->set_padding({0, 5});

    for (const std::string& keyword : std::initializer_list<std::string>{_u8L("All")}) {
        LayoutButton* keyword_button = keywords_row->emplace_back<LayoutButton>(keyword);
        keyword_button->set_checkable(true);
        keyword_button->set_rounding(5);
        keyword_button->set_content_padding({10, 5});
        m_group_keywords.insert_button(keyword_button);
    }

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    ScrollArea* scroll_area = m_page_list->emplace_back<ScrollArea>();
    scroll_area->set_max_size({YGUndefined, 275});

    // Create the ViewFactory explicitly:
    auto factory        = PrinterListViewFactory([this](size_t index) {
        m_stack_layout->set_current_index(1);
        for (size_t button_index = 0; button_index < m_printer_list_view->item_count(); ++button_index)
        {
            LogicalPrinterSettingsButton* button = dynamic_cast<LogicalPrinterSettingsButton*>(
                m_printer_list_view->get_item(button_index)
            );
            ASSERT(button);
            button->set_checked(index == button_index);
        }
        auto& preset_interactor = m_project_interactor.preset_interactor();
        const auto& item        = preset_interactor.printer_presets().items().at(index);
        preset_interactor.select_printer_preset(item.hw_printer_config_id, item.id);
    }, m_project_interactor.preset_interactor());
    m_printer_list_view = scroll_area->emplace_back<PrinterListView>(std::move(factory));
    m_printer_list_view->set_flex_grow(1);
    m_printer_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_printer_list_view->set_margin(Margins(0, 0, -10, 0));
    m_printer_list_view->set_gap(8);
    m_printer_list_view->set_orientation(Orientation::Vertical);
    m_printer_list_view->set_source_list(
        &m_project_interactor.preset_interactor().printer_presets().items()
    );

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    Item* row = m_page_list->emplace_back<Item>();
    row->set_justify_content(YGJustifySpaceBetween);

    LayoutButton* show_diff_dialog = row->emplace_back<LayoutButton>(_u8L("Compare presets"));
    show_diff_dialog->callbacks().action = [this] {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(m_project_interactor.preset_interactor());
        };
/*
    LayoutButton* add_printer_button = row->emplace_back<LayoutButton>(_u8L("Add logical printer"));
    //    add_printer_button->set_self_align(YGAlignFlexEnd);
    add_printer_button->callbacks().action = [this] {
        m_printer_add_dialog->attach_to_item(content_item(), Position::Left);
        m_printer_add_dialog->set_root_item(get_or_find_root_item());
        m_printer_add_dialog->set_current_tab(0);
        m_printer_add_dialog->open();
    };*/
}

void Slic3r::App::PrinterSettingsDialog::create_page_settings()
{
    m_page_settings = m_stack_layout->emplace_back<Item>();
    m_page_settings->set_orientation(Orientation::Vertical);
    m_page_settings->set_gap(5);
    m_page_settings->set_padding(10);

    Item* title_row           = m_page_settings->emplace_back<Item>();
    LayoutButton* back_button = title_row->emplace_back<LayoutButton>("", Render::Icon::CaretLeft);
    back_button->callbacks().action = [this]() {
        m_stack_layout->set_current_index(0);
    };
    m_text_printer_name = title_row->emplace_back<Text>("Unknown");

    m_page_settings->emplace_back<Separator>(Orientation::Horizontal);

    m_printer_icon = m_page_settings->emplace_back<Icon>(Render::Icon::PrinterNEXT);
    m_printer_icon->set_height(225);
    m_printer_icon->set_margin({0, 5});
    m_printer_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);

    m_page_settings->emplace_back<Text>(_u8L("Sheet"), Render::ImguiFontType::Bold);
    m_combo_sheets = m_page_settings->emplace_back<
        Yoga::ComboBoxListViewSelection<Domain::Preset::HwSheetConfigDef>>();
    m_combo_sheets->set_get_name_fn([](const Domain::Preset::HwSheetConfigDef* data) -> std::string {
        return data->name;
    });
    m_combo_sheets->set_source_list(&m_project_interactor.preset_interactor().sheet_items().items());
    m_combo_sheets->callbacks().selection_changed = [this](int sheet_index) {
        if (sheet_index >= 0) {
            m_project_interactor.preset_interactor().select_printer_sheet(
                m_project_interactor.preset_interactor().sheet_items().items().at(sheet_index).id
            );
        }
    };
    m_project_interactor.preset_interactor()
        .sheet_items()
        .add_listener<Biz::IListSelectionChangedListener>(m_combo_sheets);

    Text* label = m_page_settings->emplace_back<Text>(_u8L("Nozzles"), Render::ImguiFontType::Bold);
    label->set_margin(Margins(0, 10, 0, 0));

    m_nozzle_list_view = m_page_settings->emplace_back<NozzleListView>(
        m_project_interactor.preset_interactor()
    );
    m_nozzle_list_view->set_orientation(Orientation::Vertical);
    m_nozzle_list_view->set_gap(5);
    m_nozzle_list_view->set_source_list(&m_project_interactor.preset_interactor().tool_items());

    m_advanced_dialog.attach_to_item(content_item(), Position::Left);

    LayoutButton* button_advanced_setting = m_page_settings->emplace_back<LayoutButton>(
        _u8L("Advanced settings")
    );
    button_advanced_setting->set_checkable(true);
    button_advanced_setting->callbacks().checked_changed = [this](bool checked) {
        if (checked) {
            m_advanced_dialog.set_root_item(get_or_find_root_item());
            m_advanced_dialog.open();
        } else {
            m_advanced_dialog.close();
        }
    };
}

void Slic3r::App::PrinterSettingsDialog::on_about_to_show()
{
    m_stack_layout->set_current_index(0);
}
