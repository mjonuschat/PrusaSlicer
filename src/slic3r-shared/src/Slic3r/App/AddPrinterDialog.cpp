#include "Slic3r/App/AddPrinterDialog.hpp"
#include "Slic3r/App/PrinterSearchFunction.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

using namespace Yoga;

static Yoga::Item* append_item(Yoga::Item* parent, Yoga::ItemPtr item)
{
    auto item_ptr{item.get()};
    parent->append(std::move(item));
    return item_ptr;
}

struct AddPrinterDialog::PrinterFamily
{
    std::string base_model;
    std::vector<AddPrinterDialog::Printer> printers;
};

static std::string get_thumbnail(const Domain::Preset::HwPrinterConfig& config)
{
    if (!config.visual.thumbnail) {
        return "";
    }
    return config.relative_path_to_assets() + *config.visual.thumbnail;
}

static std::vector<std::string> to_strings(
    const std::vector<Domain::Preset::HwSheetConfigDef>& sheets)
{
    std::vector<std::string> result;
    result.reserve(sheets.size());
    std::ranges::transform(sheets,
                           std::back_inserter(result),
                           [](const Domain::Preset::HwSheetConfigDef& sheet)
                           { return sheet.name; });
    return result;
}

static std::vector<std::string> to_strings(
    const std::vector<Domain::Preset::HwToolConfigDef>& tools)
{
    std::vector<std::string> result;
    result.reserve(tools.size());
    std::ranges::transform(tools,
                           std::back_inserter(result),
                           [](const Domain::Preset::HwToolConfigDef& tool) { return tool.name; });
    return result;
}

class PrinterDetail : public Item
{
public:
    PrinterDetail(const AddPrinterDialog::Printer& printer_settings,
                  const std::function<void()>& go_back,
                  const std::function<void(const AddPrinterDialog::Printer&)>& add_printer)
    {
        set_orientation(Orientation::Vertical);
        set_width_percent(100);
        set_align_items(YGAlignStretch);

        auto back_button_row{emplace_back<Item>()};
        back_button_row->set_flex_shrink(0);

        auto back_button{back_button_row->emplace_back<LayoutButton>("Back to printers")};
        back_button->set_background_color(Platform::Color::Transparent);
        back_button->set_rounding(0);
        back_button->set_icon(Render::Icon::ChevronLeft);
        back_button->set_padding({20_fpx, 16_fpx, 20_fpx, 16_fpx});
        back_button->callbacks().action = go_back;

        auto separator{emplace_back<Rectangle>()};
        separator->set_height(1_px);
        separator->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));

        auto detail{emplace_back<ScrollArea>()};
        detail->set_self_align(YGAlignStretch);

        detail->set_padding(20_fpx);
        detail->set_flex_wrap(YGWrapWrap);
        detail->set_gap(20_fpx);
        detail->set_flex_grow(1);

        auto image{detail->emplace_back<Icon>(Render::Icon::None)};
        image->set_width(120_fpx);
        image->set_aspect_ratio(1);
        image->set_image(get_thumbnail(printer_settings.default_config));
        image->set_flex_shrink(0);

        auto settings{detail->emplace_back<Item>()};
        settings->set_flex_shrink(0);
        settings->set_gap(10_fpx);
        settings->set_orientation(Orientation::Vertical);
        auto title{settings->emplace_back<Text>(printer_settings.default_config.short_name)};
        title->set_font_size(18_fpx);
        title->set_font_type(Render::ImguiFontType::Bold);

        auto sheet_section{settings->emplace_back<Item>()};
        sheet_section->set_gap(5_fpx);
        sheet_section->set_flex_shrink(0);
        sheet_section->set_orientation(Orientation::Vertical);
        auto sheet_label{sheet_section->emplace_back<Text>(Biz::_u8L("Sheet"))};
        sheet_label->set_font_type(Render::ImguiFontType::Bold);

        auto sheet_combo{sheet_section->emplace_back<ComboBox>("")};
        sheet_combo->set_min_width(200_fpx);
        sheet_combo->set_items(to_strings(printer_settings.sheets.sheet_defs));
        sheet_combo->set_current_index(printer_settings.sheets.selected_sheet);

        auto nozzle_section{settings->emplace_back<Item>()};
        nozzle_section->set_gap(5_fpx);
        nozzle_section->set_flex_shrink(0);
        nozzle_section->set_orientation(Orientation::Vertical);
        auto nozzle_label{nozzle_section->emplace_back<Text>(
            printer_settings.default_config.tool_count > 1 ? Biz::_u8L("Nozzles") :
                                                             Biz::_u8L("Nozzle"))};
        nozzle_label->set_font_type(Render::ImguiFontType::Bold);

        std::size_t index{1};
        std::vector<ComboBox*> nozzle_combos;
        for (const std::size_t selected_index : printer_settings.tools.selected_tools) {
            auto nozzle_row{nozzle_section->emplace_back<Item>()};
            nozzle_row->set_align_items(YGAlignStretch);
            if (printer_settings.default_config.tool_count > 1) {
                auto rectangle{nozzle_row->emplace_back<Rectangle>()};
                rectangle->emplace_back<Text>(std::to_string(index++));
                rectangle->set_width(25_fpx);
                rectangle->set_height(25_fpx);
                rectangle->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));
                rectangle->set_justify_content(YGJustifyCenter);
                rectangle->set_align_items(YGAlignCenter);
            }
            nozzle_combos.push_back(nozzle_row->emplace_back<ComboBox>(""));
            nozzle_combos.back()->set_items(to_strings(printer_settings.tools.tool_defs));
            nozzle_combos.back()->set_min_width(
                printer_settings.default_config.tool_count > 1 ? 175_fpx : 200_fpx);
            nozzle_combos.back()->set_current_index(selected_index);
        }

        separator = emplace_back<Rectangle>();
        separator->set_height(1_px);
        separator->set_fill(m_theme->color_imgui(Platform::Color::WindowBgAlternate));

        auto actions{emplace_back<Item>()};
        actions->set_flex_shrink(0);
        actions->set_justify_content(YGJustifyFlexEnd);
        actions->set_align_items(YGAlignCenter);
        actions->set_padding(20_fpx);
        auto add_button{actions->emplace_back<LayoutButton>(Biz::_u8L("Add printer"))};
        add_button->set_background_color(Platform::Color::AccentPrimary);
        add_button->set_content_padding({20_fpx, 10_fpx, 20_fpx, 10_fpx});

        add_button->callbacks().action =
            [printer_settings, add_printer, sheet_combo, nozzle_combos]()
        {
            AddPrinterDialog::Printer printer{printer_settings};
            printer.sheets.selected_sheet = sheet_combo->current_index();
            std::vector<std::size_t> tool_indices;
            for (const ComboBox* combo_box : nozzle_combos) {
                tool_indices.push_back(combo_box->current_index());
            }
            printer.tools.selected_tools = tool_indices;
            add_printer(printer);
        };
    }
};

AddPrinterDialog::AddPrinterDialog(Biz::ProjectInteractor& project_interactor,
                                   std::function<void()> close,
                                   std::function<void(const Printer&)> add_printer) :
    m_project_interactor{project_interactor},
    m_searcher{score_printer},
    m_add_printer{add_printer}
{
    const ImColor secondary_color{m_theme->color_imgui(Platform::Color::WindowBgAlternate)};

    set_orientation(Orientation::Vertical);
    set_align_items(YGAlignStretch);
    auto top_bar{emplace_back<Rectangle>()};
    top_bar->set_align_items(YGAlignCenter);
    top_bar->set_padding({0, 0, 10_fpx, 0});
    top_bar->set_fill(secondary_color);
    top_bar->set_flex_shrink(0);
    top_bar->set_justify_content(YGJustifySpaceBetween);
    auto title{top_bar->emplace_back<Rectangle>()};
    title->set_padding({30_fpx, 11_fpx, 30_fpx, 11_fpx});
    title->emplace_back<Text>(Biz::_u8L("Choose a printer"));
    title->set_fill(m_theme->color_imgui(Platform::Color::WindowBg));
    title->set_rounding(0);
    title->set_flex_shrink(0);

    auto close_button{top_bar->emplace_back<LayoutButton>("", Render::Icon::TopBarCross)};
    close_button->set_width(22_fpx);
    close_button->set_height(22_fpx);
    close_button->callbacks().action = close;

    auto search_row{emplace_back<Item>()};
    search_row->set_gap(14_fpx);
    search_row->set_align_items(YGAlignCenter);
    search_row->set_padding({38_fpx, 6_fpx, 6_fpx, 6_fpx});
    auto search_icon{search_row->emplace_back<Icon>(Render::Icon::Search)};
    search_icon->set_width(16_fpx);
    search_icon->set_height(16_fpx);
    search_row->set_flex_shrink(0);

    auto search{search_row->emplace_back<InputTextField>()};
    search->set_padding(8_fpx);
    search->set_flex_shrink(0);
    search->set_color(Platform::Color::WindowBg);
    search->set_rounding(0);
    search->set_hint(Biz::_u8L("Search printer"));
    search->set_flex_grow(1);
    search->callbacks().text_changed = [this, search]
    { m_searcher.set_search_text(search->text()); };

    auto separator{emplace_back<Rectangle>()};
    separator->set_fill(secondary_color);
    separator->set_height(1_px);
    separator->set_flex_shrink(0);

    auto content{emplace_back<Item>()};
    content->set_flex_grow(1);
    content->set_align_items(YGAlignStretch);

    auto left_bar{content->emplace_back<ScrollArea>()};
    left_bar->set_width(240_fpx);
    left_bar->set_padding({0, 20_fpx, 0, 20_fpx});
    left_bar->set_orientation(Orientation::Vertical);

    auto vertical_separator{content->emplace_back<Rectangle>()};
    vertical_separator->set_fill(secondary_color);
    vertical_separator->set_width(1_px);

    auto source{left_bar->emplace_back<LayoutButton>("Prusa3D")};
    source->set_content_padding({20_fpx, 10_fpx, 20_fpx, 10_fpx});
    source->set_rounding(0);
    source->set_checkable(true);
    source->set_checked(true);
    source->set_content_justify_content(YGJustifyFlexStart);

    m_printers = content->emplace_back<ScrollArea>();
    m_printers->set_flex_grow(1);
    m_printers->set_padding(20_fpx);
    m_printers->set_orientation(Orientation::Vertical);
    m_printers->set_gap(35_fpx);

    m_detail = content->emplace_back<Item>();
    m_detail->set_width_percent(100);
    m_detail->set_visible(false);
    m_printers->set_flex_grow(1);

    m_searcher.add_listener<Biz::IListObserver<Biz::Preset::PresetItem>>(this);
    m_searcher.set_source_model(&project_interactor.preset_interactor().printer_presets().items());
}

AddPrinterDialog::~AddPrinterDialog()
{
    m_searcher.remove_listener<Biz::IListObserver<Biz::Preset::PresetItem>>(this);
}

ItemPtr AddPrinterDialog::create_printer_family(const PrinterFamily& printer_family)
{
    auto result{std::make_unique<Item>()};
    result->set_orientation(Orientation::Vertical);
    result->set_gap(20_fpx);
    result->set_flex_shrink(0);

    auto title_item{result->emplace_back<Text>(printer_family.base_model)};
    title_item->set_font_type(Render::ImguiFontType::Bold);
    title_item->set_font_size(16_fpx);
    title_item->set_flex_shrink(0);

    auto printer_section{result->emplace_back<Item>()};
    printer_section->set_gap(20_fpx);
    printer_section->set_flex_wrap(YGWrapWrap);
    printer_section->set_flex_shrink(0);

    for (const AddPrinterDialog::Printer& printer_settings : printer_family.printers) {
        auto printer{printer_section->emplace_back<RectangleButton>()};
        printer->set_flex_shrink(0);
        printer->set_content_padding(0);
        printer->callbacks().action = [this, printer_settings]()
        { show_printer_detail(printer_settings); };

        auto printer_content{printer->emplace_back<Item>()};
        printer_content->set_padding({20_fpx, 15_fpx, 20_fpx, 15_fpx});
        printer_content->set_width(160_fpx);
        printer_content->set_height(140_fpx);
        printer_content->set_orientation(Orientation::Vertical);
        printer_content->set_gap(15_fpx);
        printer_content->set_flex_shrink(0);
        printer_content->set_align_items(YGAlignCenter);

        auto image{printer_content->emplace_back<Icon>(Render::Icon::None)};
        image->set_image(get_thumbnail(printer_settings.default_config));
        image->set_height(80_fpx);
        image->set_aspect_ratio(1.0);
        image->set_flex_shrink(0);

        printer_content->emplace_back<Text>(printer_settings.default_config.short_name);
    }
    return result;
}

void AddPrinterDialog::reload()
{
    m_detail->set_visible(false);
    m_printers->set_visible(true);
    std::vector<PrinterFamily> printer_families;

    for (std::size_t i{}; i < m_searcher.size(); ++i) {
        const Biz::Preset::PresetItem& preset_item{m_searcher.at(i)};

        const Domain::Preset::HwPrinterConfig& config{
            m_project_interactor.preset_interactor()
                .get_printer_config(m_project_interactor.selected_project_id(),
                                    preset_item.hw_printer_config_id)
                .first.get()};

        const auto it{
            std::ranges::find_if(printer_families,
                                 [&](const PrinterFamily& family)
                                 { return family.base_model == config.model.base_model; })};

        const AddPrinterDialog::Printer printer_settings{
            .preset_item_id = preset_item.id,
            .default_config = config,
            .tools          = m_project_interactor.preset_interactor().get_tool_items(config),
            .sheets         = m_project_interactor.preset_interactor().get_sheet_items(config)};

        if (it == printer_families.end()) {
            printer_families.push_back(PrinterFamily{.base_model = config.model.base_model,
                                                     .printers   = {printer_settings}});
        } else {
            it->printers.push_back(printer_settings);
        }
    }

    while (!m_printers->items().empty()) {
        m_printers->remove(m_printers->items().back());
    }

    for (const PrinterFamily& printer_family : printer_families) {
        append_item(m_printers, create_printer_family(printer_family));
    }
}

void AddPrinterDialog::show_printer_detail(const AddPrinterDialog::Printer& printer_settings)
{
    m_printers->set_visible(false);
    m_detail->set_visible(true);

    while (!m_detail->items().empty()) {
        m_detail->remove(m_detail->items().back());
    }

    const auto go_back{[this]()
                       {
                           m_printers->set_visible(true);
                           m_detail->set_visible(false);
                       }};

    m_detail->emplace_back<PrinterDetail>(printer_settings,
                                          go_back,
                                          [this, go_back](const Printer& printer)
                                          {
                                              go_back();
                                              m_add_printer(printer);
                                          });
}

} // namespace Slic3r::App
