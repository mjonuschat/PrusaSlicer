///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterSettingsDialog.hpp"

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"

using namespace Slic3r::App::Yoga;

namespace {

void emplace_nozzle(Item* container, const std::string& id)
{
    Item* row = container->emplace_back<Item>();
    row->set_flex_grow(1);

    Rectangle* id_background = row->emplace_back<Rectangle>();
    id_background->set_fill(ImColor(41, 41, 41));
    id_background->set_flags(ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft);
    id_background->set_align_items(YGAlignCenter);
    id_background->set_justify_content(YGJustifyCenter);
    id_background->set_width(25);

    id_background->emplace_back<Text>(id);

    ComboBox* combo_width = row->emplace_back<ComboBox>(
        std::initializer_list<std::string>{"0.25 mm", "0.4 mm", "0.6 mm", "0.8 mm"}
    );
    combo_width->set_flex_grow(1);

    ComboBox* combo_type = row->emplace_back<ComboBox>(
        std::initializer_list<std::string>{"Standard", "High-flow"}
    );
    combo_type->set_flex_grow(1);
}

} // namespace

Slic3r::App::PrinterSettingsDialog::PrinterSettingsDialog(PrinterAddDialog* printer_add_dialog)
    : Dialog("Printers"), m_printer_add_dialog(printer_add_dialog)
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

void Slic3r::App::PrinterSettingsDialog::create_page_list()
{
    m_page_list = m_stack_layout->emplace_back<Item>();
    m_page_list->set_orientation(Orientation::Vertical);
    m_page_list->set_gap(5);
    m_page_list->set_padding(10);

    Item* keywords_row = m_page_list->emplace_back<Item>();
    keywords_row->set_padding({0, 5});

    for (const std::string& keyword : std::initializer_list<std::string>{"All"}) {
        LayoutButton* keyword_button = keywords_row->emplace_back<LayoutButton>(keyword);
        keyword_button->set_checkable(true);
        keyword_button->set_rounding(5);
        keyword_button->set_content_padding({10, 5});
        m_group_keywords.insert_button(keyword_button);
    }

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    ScrollArea* scroll_area = m_page_list->emplace_back<ScrollArea>();
    scroll_area->set_max_size({YGUndefined, 200});

    m_list_logical_printers.reset(
        {{"Prusa", "Next"},
         {"Prusa", "Core one"},
         {"Prusa", "Mk4S"},
         {"Prusa", "Mk3"},
         {"Prusa", "Mini+"},
         {"Creality", "Ender 3"}}
    );

    // Create the ViewFactory explicitly:
    auto factory = Yoga::ViewFactory<
        LogicalPrinterSettingsButton, LogicalPrinter, LogicalPrinterSettingsButton::FnIndexClicked>(
        [this](size_t index) {
            m_stack_layout->set_current_index(1);
            for (size_t button_index = 0; button_index < m_printer_list_view->item_count();
                 ++button_index) {
                LogicalPrinterSettingsButton* button = dynamic_cast<LogicalPrinterSettingsButton*>(
                    m_printer_list_view->get_item(button_index)
                );
                ASSERT(button);
                button->set_checked(index == button_index);
            }
        }
    );
    m_printer_list_view = scroll_area->emplace_back<PrinterListView>(std::move(factory));
    m_printer_list_view->set_flex_grow(1);
    m_printer_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_printer_list_view->set_margin(Margins(0, 0, -10, 0));
    m_printer_list_view->set_gap(8);
    m_printer_list_view->set_orientation(Orientation::Vertical);
    m_printer_list_view->set_source_list(&m_list_logical_printers);

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    LayoutButton* add_printer_button = m_page_list->emplace_back<LayoutButton>("Add logical printer");
    add_printer_button->set_self_align(YGAlignFlexEnd);
    add_printer_button->callbacks().action = [this] {
        m_printer_add_dialog->attach_to_item(content_item(), Position::Left);
        m_printer_add_dialog->set_root_item(root_item());
        m_printer_add_dialog->set_current_tab(0);
        m_printer_add_dialog->open();
    };
}

void Slic3r::App::PrinterSettingsDialog::create_page_settings()
{
    m_page_settings = m_stack_layout->emplace_back<Item>();
    m_page_settings->set_orientation(Orientation::Vertical);
    m_page_settings->set_gap(5);
    m_page_settings->set_padding(10);

    Item* title_row = m_page_settings->emplace_back<Item>();
    LayoutButton* back_button = title_row->emplace_back<LayoutButton>("", Render::Icon::CaretLeft);
    back_button->callbacks().action = [this]() { m_stack_layout->set_current_index(0); };
    title_row->emplace_back<Text>("NEXT / Elsa");

    m_page_settings->emplace_back<Separator>(Orientation::Horizontal);

    Icon* printer_icon = m_page_settings->emplace_back<Icon>(Render::Icon::PrinterNEXT);
    printer_icon->set_height(150);
    printer_icon->set_margin({0, 10});
    printer_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);

    m_page_settings->emplace_back<Text>("Sheet", Render::ImguiFontType::Bold);
    m_page_settings->emplace_back<ComboBox>(
        std::initializer_list<std::string>{"Satin", "Smooth", "Textured"}
    );

    Text* label = m_page_settings->emplace_back<Text>("Nozzles", Render::ImguiFontType::Bold);
    label->set_margin(Margins(0, 10, 0, 0));

    emplace_nozzle(m_page_settings, "1");
    emplace_nozzle(m_page_settings, "2");
    emplace_nozzle(m_page_settings, "3");
    emplace_nozzle(m_page_settings, "4");

    m_advanced_dialog.attach_to_item(content_item(), Position::Left);

    LayoutButton* button_advanced_setting = m_page_settings->emplace_back<LayoutButton>(
        "Advanced settings"
    );
    button_advanced_setting->set_checkable(true);
    button_advanced_setting->callbacks().checked_changed = [this](bool checked) {
        if (checked) {
            m_advanced_dialog.set_root_item(root_item());
            m_advanced_dialog.open();
        } else {
            m_advanced_dialog.close();
        }
    };
}
