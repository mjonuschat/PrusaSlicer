///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"
#include "Slic3r/App/PhysicalPrinterAdvancedSettingsDialog.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"
#include "Slic3r/App/Navigator.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;
using namespace Slic3r::Biz::PhysicalPrinter;

namespace Slic3r::App {

PhysicalPrinterSettingsDialog::PhysicalPrinterSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    PrinterAddDialog* printer_add_dialog,
    Navigator& navigator
) :
    Dialog({"Physical printer"}, "PhysicalPrinterSettingsDialog"),
    m_physical_printer_changed_listener_scope(project_interactor.physical_printer_interactor(), *this),
    m_preset_changed_listener_scope(project_interactor.preset_interactor(), *this),
    m_printer_add_dialog(printer_add_dialog),
    m_project_interactor(project_interactor),
    m_physical_printer_interactor(project_interactor.physical_printer_interactor()),
    m_navigator(navigator)
    //,m_list_physical_printers( std::make_shared<Biz::ObservableList<Biz::PhysicalPrinter::PhysicalPrinterConfig>>())
{
    m_print_host_settings_dialog =
        emplace_back<PhysicalPrinterAdvancedSettingsDialog>(project_interactor, navigator, this);

    content_item()->set_width(350);

    content()->set_padding(0);
    content()->set_orientation(Orientation::Vertical);

    m_stack_layout = content()->emplace_back<StackLayout>();
    m_stack_layout->set_orientation(Orientation::Vertical);

    create_page_list();

    create_page_settings();

    m_stack_layout->set_current_index(0);
}

void PhysicalPrinterSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

void PhysicalPrinterSettingsDialog::create_page_list()
{
    m_page_list = m_stack_layout->emplace_back<Item>();
    m_page_list->set_orientation(Orientation::Vertical);
    m_page_list->set_gap(5);
    m_page_list->set_padding(10);

    Item* keywords_row = m_page_list->emplace_back<Item>();
    keywords_row->set_padding({0, 5});

    for (const std::string& keyword :
         std::initializer_list<std::string>{"All"/*, "Not printing", "Ready"*/})
    {
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
    auto factory = Yoga::ViewFactory<
        PhysicalPrinterSettingsButton,
        Biz::PhysicalPrinter::PhysicalPrinterConfig,
        PhysicalPrinterSettingsButton::FnIndexClicked>(
        [this](size_t index)
        {
            m_physical_printer_interactor.select_index(index);
            if (m_physical_printer_interactor.is_none_selected()) {
                close_action();
                return;
            }

            m_stack_layout->set_current_index(1);
            check_printer_button(index);
        }
    );
    m_printer_list_view = scroll_area->emplace_back<PrinterListView>(std::move(factory));
    m_printer_list_view->set_flex_grow(1);
    m_printer_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_printer_list_view->set_margin(Margins(0, 0, -10, 0));
    m_printer_list_view->set_gap(8);
    m_printer_list_view->set_orientation(Orientation::Vertical);
    m_printer_list_view->set_source_list(&m_physical_printer_interactor.observable_list());
    
    size_t selected_index = m_physical_printer_interactor.selected_index();
    PhysicalPrinterSettingsButton* button = dynamic_cast<PhysicalPrinterSettingsButton*>(
        m_printer_list_view->get_item(selected_index)
    );
    ASSERT(button);
    button->set_checked(true);
    filter_printer_buttons();

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    Item* row = m_page_list->emplace_back<Item>();
    row->set_justify_content(YGJustifySpaceBetween);

    LayoutButton* add_printer_button =
        m_page_list->emplace_back<LayoutButton>("Add physical printer");

    add_printer_button->callbacks().action = [this]
    {
        // check first item "None"
        check_printer_button(0);

        m_physical_printer_interactor.on_dialog_button_add_new();

        if (m_print_host_settings_dialog->opened()) {
            m_navigator.set_opened_dialog(this);
        } else {
            m_print_host_settings_dialog->attach_to_item(content_item(), Position::Left);
            m_navigator.set_opened_dialog(m_print_host_settings_dialog);
        }
    };
}

void PhysicalPrinterSettingsDialog::check_printer_button(size_t index)
{
    for (size_t button_index = 0; button_index < m_printer_list_view->object_count();
         ++button_index)
    {
        PhysicalPrinterSettingsButton* button = dynamic_cast<PhysicalPrinterSettingsButton*>(
            m_printer_list_view->get_item(button_index)
        );
        ASSERT(button);
        button->set_checked(button_index == index);
    }
}

void PhysicalPrinterSettingsDialog::create_page_settings()
{
    m_page_settings = m_stack_layout->emplace_back<Item>();
    m_page_settings->set_orientation(Orientation::Vertical);
    m_page_settings->set_gap(5);
    m_page_settings->set_padding(10);

    Item* name_row           = m_page_settings->emplace_back<Item>();
    LayoutButton* back_button = name_row->emplace_back<LayoutButton>("", Render::Icon::CaretLeft);
    back_button->callbacks().action = [this]() { m_stack_layout->set_current_index(0); };
    m_text_printer_name = name_row->emplace_back<Text>("Unknown");

    m_page_settings->emplace_back<Separator>(Orientation::Horizontal);

    Item* host_type_row           = m_page_settings->emplace_back<Item>();
    host_type_row->set_justify_content(YGJustifySpaceBetween);
    m_text_host_type_name = host_type_row->emplace_back<Text>("Unknown");

   m_button_advanced_setting =
        m_page_settings->emplace_back<LayoutButton>(_u8L("Advanced settings"));
    m_button_advanced_setting->callbacks().action = [this]
    {
        if (m_print_host_settings_dialog->opened()) {
            m_navigator.set_opened_dialog(this);
        } else {
            m_print_host_settings_dialog->attach_to_item(content_item(), Position::Left);
            m_navigator.set_opened_dialog(m_print_host_settings_dialog);
        }
    };

    m_button_delete =
        m_page_settings->emplace_back<LayoutButton>(_u8L("Remove"));
    m_button_delete->callbacks().action = [this]
    {
        m_stack_layout->set_current_index(0);
        m_physical_printer_interactor.remove_selected();
    };
}

void PhysicalPrinterSettingsDialog::on_printer_data_changed() 
{
    size_t index = m_physical_printer_interactor.selected_index();
    PhysicalPrinterSettingsButton* button =
        dynamic_cast<PhysicalPrinterSettingsButton*>(m_printer_list_view->get_item(index));
    ASSERT (button);
    button->update_button_text();
    button->set_icon(Render::Icon::PrinterIconMarker);

    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        m_physical_printer_interactor.selected_physical_printer_data();

    m_text_printer_name->set_text(physical_printer.name);
    m_text_host_type_name->set_text(
        std::string(Biz::PhysicalPrinter::physical_printer_type_to_string(physical_printer))
    );
}

void PhysicalPrinterSettingsDialog::on_selected_physical_printer_changed() 
{
    size_t selected_index = m_physical_printer_interactor.selected_index();
    check_printer_button(selected_index);

    const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer =
        m_physical_printer_interactor.selected_physical_printer_data();

    m_text_printer_name->set_text(physical_printer.name);
    m_text_host_type_name->set_text(
        std::string(Biz::PhysicalPrinter::physical_printer_type_to_string(physical_printer))
    );

    if (const auto* data = std::get_if<LocalAuth>(&physical_printer.connection_data); data)
    {
        m_button_advanced_setting->set_visible(true);
    } else {
        m_button_advanced_setting->set_visible(false);
    }


    // TODO: change picture
}

PhysicalPrinterAdvancedSettingsDialog& PhysicalPrinterSettingsDialog::print_host_settings_dialog()
{
    return *m_print_host_settings_dialog;
}

void PhysicalPrinterSettingsDialog::on_about_to_show()
{
    m_stack_layout->set_current_index(0);
}

void PhysicalPrinterSettingsDialog::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id
        && type == Biz::Preset::PresetItemType::PrinterPreset)
    {
        filter_printer_buttons();
    }
}

void PhysicalPrinterSettingsDialog::filter_printer_buttons()
{
    const Domain::Preset::HwPrinterConfig& printer_config =
        m_project_interactor.preset_interactor().current_printer_config();

    bool reselect_none{false};
    for (size_t i = 0; i < m_printer_list_view->object_count(); ++i) {
        PhysicalPrinterSettingsButton* button = dynamic_cast<PhysicalPrinterSettingsButton*>(
            m_printer_list_view->get_item(i)
        );
        ASSERT(button);
        bool visible{m_physical_printer_interactor.is_printer_on_index_compatible(i, printer_config)};
        if (m_physical_printer_interactor.selected_index() == i && !visible) {
            reselect_none = true;
        }
        button->set_visible(visible);
        
    }
    if (reselect_none) {
        m_physical_printer_interactor.select_index(0);
    }
}

} // namespace Slic3r::App
