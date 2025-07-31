///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterAddDialog.hpp"

#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/InputText.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

struct Printer
{
    std::string name;
    Render::Icon icon = Render::Icon::PrinterNEXT;
};

void emplace_family(Item* container, const std::string& name, const std::vector<Printer>& printers)
{
    Text* family = container->emplace_back<Text>(name, Render::ImguiFontType::Bold);
    family->set_margin(Margins(0, 10));

    Item* printers_grid = container->emplace_back<Item>();
    printers_grid->set_orientation(Orientation::Horizontal);
    printers_grid->set_gap(5);
    printers_grid->set_flex_shrink(0);
    printers_grid->set_flex_wrap(YGWrapWrap);

    for (const Printer& printer : printers) {
        LayoutButton* button =
            printers_grid->emplace_back<LayoutButton>(printer.name, printer.icon, printer.name);
        button->set_width(180);
        button->set_height(160);
        button->set_content_orientation(Orientation::Vertical);
        button->set_content_justify_content(YGJustifyCenter);
        button->set_background_color(ImColor(41, 41, 41));
    }
}

PrinterAddDialog::PrinterAddDialog() : Dialog({"Add logical printer", "Add physical printer"})
{
    content_item()->set_width(600);
    content_item()->set_height(500);

    content()->set_orientation(Orientation::Vertical);
    m_stack_layout = content()->emplace_back<StackLayout>();
    m_stack_layout->set_orientation(Orientation::Vertical);

    create_add_logical_printer_page();

    create_add_physical_printer_page();
}

PrinterAddDialog::~PrinterAddDialog() {
    m_page_list_view->set_source_list(nullptr);
}

void PrinterAddDialog::on_tab_selected(int current_index)
{
    m_stack_layout->set_current_index(current_index);
}

void PrinterAddDialog::create_add_logical_printer_page()
{
    Item* logical_printer_page = m_stack_layout->emplace_back<Item>();
    logical_printer_page->set_orientation(Orientation::Vertical);
    logical_printer_page->set_gap(5);

    Item* search_row = logical_printer_page->emplace_back<Item>();
    search_row->set_gap(5);
    search_row->set_flex_shrink(0);

    Icon* icon = search_row->emplace_back<Icon>(Render::Icon::Search);
    icon->set_width(20);
    icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
    InputText* search_input = search_row->emplace_back<InputText>();
    search_input->set_flex_grow(1);
    search_input->set_hint("Search printer");

    for (const std::string& search_type : std::initializer_list<std::string>{"All", "FFF", "SLA"}) {
        LayoutButton* search_button = search_row->emplace_back<LayoutButton>(search_type);
        search_button->set_rounding(10);
        search_button->set_checkable(true);
        search_button->set_background_color(ImColor(61, 61, 61));
        search_button->set_content_padding({15, 2});
        m_group_search.insert_button(search_button);
        if (search_type == "All") {
            search_button->set_checked(true);
        }
    }

    logical_printer_page->emplace_back<Separator>(Orientation::Horizontal);

    Item* layout_logic_row = logical_printer_page->emplace_back<Item>();

    m_list_vendors.reset({
        {"Prusa3D"},
        {"Prusa PRO"},
        {"AnkerMake"},
        {"AnyCubic"},
        {"Artillery"},
        {"BIBO"},
        {"BIQU"},
        {"Cocoa Press"},
        {"Creality"},
        {"E3D"},
        {"Elegoo"},
    });

    auto factory = Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>(
        [this](size_t index) {
            // m_pages_stack_layout->set_current_index(index);
            for (size_t button_index = 0; button_index < m_page_list_view->item_count();
                 ++button_index) {
                PageEntryButton* button = dynamic_cast<PageEntryButton*>(
                    m_page_list_view->get_item(button_index)
                );
                ASSERT(button);
                button->set_checked(index == button_index);
            }
        }
    );
    m_page_list_view = layout_logic_row->emplace_back<PageListView>(std::move(factory));
    m_page_list_view->set_orientation(Orientation::Vertical);
    m_page_list_view->set_min_size({125, 0});
    m_page_list_view->set_source_list(&m_list_vendors);
    dynamic_cast<AbstractButton*>(m_page_list_view->get_item(0))->set_checked(true);

    layout_logic_row->emplace_back<Separator>(Orientation::Vertical);

    ScrollArea* printers = layout_logic_row->emplace_back<ScrollArea>();
    printers->set_orientation(Orientation::Vertical);
    printers->set_gap(5);
    printers->set_flex_grow(1);
    printers->set_padding(10);

    emplace_family(printers, "NEXT Family", {{"NEXT"}});
    emplace_family(printers, "Core Family", {{"Core one"}, {"Core one+"}});
    emplace_family(printers, "MK4 Family", {{"MK4S"}, {"MK4"}, {"MK4mk2"}});
}

void PrinterAddDialog::create_add_physical_printer_page()
{
    Item* physical_printer_page = m_stack_layout->emplace_back<Item>();
    physical_printer_page->set_orientation(Orientation::Vertical);
    physical_printer_page->set_gap(5);

    physical_printer_page->emplace_back<Text>("IP", Render::ImguiFontType::Bold);
    physical_printer_page->emplace_back<InputTextField>();

    physical_printer_page->emplace_back<Text>("Password", Render::ImguiFontType::Bold);
    physical_printer_page->emplace_back<InputTextField>();

    LayoutButton* connect_button = physical_printer_page->emplace_back<LayoutButton>("Connect");
    connect_button->set_self_align(YGAlignFlexEnd);
}

} // namespace Slic3r::App
