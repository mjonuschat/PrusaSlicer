///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PhysicalPrinterSettingsDialog.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PhysicalPrinterSettingsDialog::PhysicalPrinterSettingsDialog(PrinterAddDialog* printer_add_dialog) :
    Dialog({"Physical printer"}, "PhysicalPrinterSettingsDialog"),
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

PhysicalPrinterSettingsDialog::~PhysicalPrinterSettingsDialog()
{
    m_printer_list_view->set_source_list(nullptr);
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
         std::initializer_list<std::string>{"All", "Not printing", "Ready"})
    {
        LayoutButton* keyword_button = keywords_row->emplace_back<LayoutButton>(keyword);
        keyword_button->set_checkable(true);
        keyword_button->set_rounding(5);
        keyword_button->set_content_padding({10, 5});
        m_group_keywords.insert_button(keyword_button);
    }

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    ScrollArea* scroll_area = m_page_list->emplace_back<ScrollArea>();
    scroll_area->set_max_size({YGUndefined, 300});

    m_list_physical_printers.reset(
        {{"Mia", "Core one"},
         {"Isabella", "Core one"},
         {"Elsa", "Next"},
         {"Francesca", "Next"},
         {"Lucia", "Next"},
         {"Bea", "Mk4S"}}
    );

    // Create the ViewFactory explicitly:
    auto factory = Yoga::ViewFactory<
        PhysicalPrinterSettingsButton,
        PhysicalPrinter,
        PhysicalPrinterSettingsButton::FnIndexClicked>([this](size_t index) {
        m_stack_layout->set_current_index(1);
        for (size_t button_index = 0; button_index < m_printer_list_view->item_count(); ++button_index)
        {
            PhysicalPrinterSettingsButton* button = dynamic_cast<PhysicalPrinterSettingsButton*>(
                m_printer_list_view->get_item(button_index)
            );
            ASSERT(button);
            button->set_checked(index == button_index);
        }
    });
    m_printer_list_view = scroll_area->emplace_back<PrinterListView>(std::move(factory));
    m_printer_list_view->set_flex_grow(1);
    m_printer_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_printer_list_view->set_margin(Margins(0, 0, -10, 0));
    m_printer_list_view->set_gap(8);
    m_printer_list_view->set_orientation(Orientation::Vertical);
    m_printer_list_view->set_source_list(&m_list_physical_printers);

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    LayoutButton* add_printer_button = m_page_list->emplace_back<LayoutButton>("Add physical printer");
    add_printer_button->set_self_align(YGAlignFlexEnd);
    add_printer_button->callbacks().action = [this] {
        m_printer_add_dialog->attach_to_item(content_item(), Position::Left);
        m_printer_add_dialog->set_root_item(get_or_find_root_item());
        m_printer_add_dialog->set_current_tab(1);
        m_printer_add_dialog->open();
    };
}

void PhysicalPrinterSettingsDialog::create_page_settings()
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
    title_row->emplace_back<Text>("NEXT / Elsa");

    m_page_settings->emplace_back<Separator>(Orientation::Horizontal);

    Icon* printer_icon = m_page_settings->emplace_back<Icon>(Render::Icon::PrinterNEXT);
    printer_icon->set_height(150);
    printer_icon->set_margin({0, 10});
    printer_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);

    m_page_settings->emplace_back<Text>("IP", Render::ImguiFontType::Bold);
    m_page_settings->emplace_back<InputTextField>();

    m_page_settings->emplace_back<Text>("Password", Render::ImguiFontType::Bold);
    m_page_settings->emplace_back<InputTextField>();

    LayoutButton* connect_button = m_page_settings->emplace_back<LayoutButton>("Connect");
    connect_button->set_self_align(YGAlignFlexEnd);
}

} // namespace Slic3r::App
