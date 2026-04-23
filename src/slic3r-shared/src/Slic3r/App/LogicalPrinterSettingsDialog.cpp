///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/LogicalPrinterSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Preset/PresetSelectionCheck.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Yoga/InputText.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/PrinterAddDialog.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfigInteractor.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App {

LogicalPrinterSettingsDialog::LogicalPrinterSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    PrinterAddDialog* printer_add_dialog,
    Navigator& navigator
) :
    Dialog({"Printers"}, "LogicalPrinterSettingsDialog"),
    m_preset_changed_listener_scope(project_interactor.preset_interactor(), *this),
    m_preset_list_selection_changed_listener_scope(
        project_interactor.preset_interactor().printer_presets(),
        *this
    ),
    m_selected_project_changed_listener_scope(project_interactor, *this),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_preset_favorite_filter(
        std::make_shared<Biz::ObservableListSortFilter<Biz::Preset::PresetItem>>()
    ),
    m_preset_searcher(
        std::make_shared<Biz::ObservableListSearcher<Biz::Preset::PresetItem>>(
            [this](const Biz::Preset::PresetItem& item, const std::string& search_text) -> int
            {
                int score = 0;

                const std::string& name = m_preset_searcher->processed_string(item.name);
                if (!name.empty() && name.find(search_text) != std::string::npos) {
                    score += 10;
                }

                const std::string& hw_printer_config_name =
                    m_preset_searcher->processed_string(item.hw_printer_config_name);
                if (!hw_printer_config_name.empty()
                    && hw_printer_config_name.find(search_text) != std::string::npos)
                {
                    score += 10;
                }

                return score;
            }
        )
    ),
    m_printer_add_dialog(printer_add_dialog)
{
    m_preset_favorite_filter->set_filter_fn(
        [this](const Biz::Preset::PresetItem& item) -> bool
        {
            return m_only_favorites_button->checked() ?
                AppServices::instance()
                    .app_config()
                    .app_settings_advanced()
                    .contains_printer_favorite_preset(item.id, item.hw_printer_config_id) :
                true;
        }
    );

    m_advanced_dialog = content_item()->emplace_back<PrinterAdvancedSettingsDialog>(
        project_interactor,
        m_navigator,
        this
    );

    content_item()->set_width(350);

    content()->set_padding(0);
    content()->set_orientation(Orientation::Vertical);

    m_stack_layout = content()->emplace_back<StackLayout>();
    m_stack_layout->set_orientation(Orientation::Vertical);

    create_page_list();

    create_page_settings();

    m_stack_layout->set_current_index(0);

    // Default project is already loaded, update default printer selection
    on_list_selection_changed(
        m_project_interactor.preset_interactor().printer_presets().selected_index()
    );
}

void LogicalPrinterSettingsDialog::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (m_project_interactor.selected_project_id() == project_id
        && m_project_interactor.selected_config_container_id() == config_container_id
        && type == Biz::Preset::PresetItemType::PrinterPreset)
    {
        update_settings_data();
    }
}

void LogicalPrinterSettingsDialog::on_config_container_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    m_warning->set_visible(false);
}

void LogicalPrinterSettingsDialog::on_list_selection_changed(Domain::SelectionId new_selection)
{
    if (new_selection == Domain::INVALID_ID) {
        return;
    }

    for (size_t button_index = 0; button_index < m_printer_list_view->object_count();
         ++button_index)
    {
        LogicalPrinterSettingsButton* button = dynamic_cast<LogicalPrinterSettingsButton*>(
            m_printer_list_view->get_item(button_index)
        );
        ASSERT(button);
        button->set_checked(new_selection == button_index);
    }

    update_settings_data();
}

void LogicalPrinterSettingsDialog::on_selected_project_changed_final(size_t index)
{
    if (index == Domain::INVALID_ID) {
        return;
    }

    // Once we will start to implement remembering Dialog context in Navigator
    // This should be cleaned up to open proper page instead of defaulting to list
    m_stack_layout->set_current_index(0);
    m_warning->set_visible(m_project_interactor.preset_interactor().has_invalid_hw_config());
}

PrinterAdvancedSettingsDialog& LogicalPrinterSettingsDialog::printer_advanced_settings_dialog()
{
    return *m_advanced_dialog;
}

void LogicalPrinterSettingsDialog::select_page_settings()
{
    m_stack_layout->set_current_index(1);
}

void LogicalPrinterSettingsDialog::on_app_config_changed(const std::string& key)
{
    if (key == "printers_only_favorites") {
        m_only_favorites_button->set_checked(
            AppServices::instance()
                .app_config()
                .get_config_box()
                .items.find("printers_only_favorites")
                ->value()
                .get<bool>()
        );
    }
}

void LogicalPrinterSettingsDialog::create_page_list()
{
    m_page_list = m_stack_layout->emplace_back<Item>();
    m_page_list->set_orientation(Orientation::Vertical);
    m_page_list->set_gap(5);
    m_page_list->set_padding(10);

    Item* search_row = m_page_list->emplace_back<Item>();
    search_row->set_gap(5);
    Icon* icon = search_row->emplace_back<Icon>(Render::Icon::Search);
    icon->set_width(16);
    icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
    m_input_text_search = search_row->emplace_back<InputText>();
    m_input_text_search->set_hint(_u8L("Search..."));
    m_input_text_search->set_flex_grow(1);
    m_input_text_search->callbacks().text_changed = [this]
    { m_preset_searcher->set_search_text(m_input_text_search->text()); };

    m_only_favorites_button = search_row->emplace_back<LayoutButton>(
        std::string{},
        Render::Icon::Star,
        Biz::_u8L("Only favorites")
    );
    m_only_favorites_button->set_width(24);
    m_only_favorites_button->set_height(24);
    m_only_favorites_button->set_checkable(true);
    m_only_favorites_button->set_self_align(YGAlignCenter);
    m_only_favorites_button->callbacks().checked_changed = [this](bool checked)
    {
        m_only_favorites_button->set_icon(checked ? Render::Icon::StarSolid : Render::Icon::Star);
        m_only_favorites_button->set_tooltip(
            checked ? Biz::_u8L("Show all items") : Biz::_u8L("Filter only favorited items")
        );
        m_preset_favorite_filter->invalidate();
        AppServices::instance().app_config_interactor().set_item_value(
            "printers_only_favorites",
            Domain::ConfigValue{checked},
            0
        );
    };
    on_app_config_changed("printers_only_favorites");

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    ScrollArea* scroll_area = m_page_list->emplace_back<ScrollArea>();
    scroll_area->set_max_size({YGUndefined, 275});
    scroll_area->set_min_size({0, 275});
    scroll_area->set_margin({0.f, 0.f, -10.f, 0.f});
    scroll_area->set_padding({0.f, 0.f, 15.f, 0.f});

    // Create the ViewFactory explicitly:
    auto factory = PrinterListViewFactory(
        [this](size_t index)
        {
            auto& preset_interactor = m_project_interactor.preset_interactor();
            const Biz::Preset::PresetItem& item =
                m_printer_list_view->item_at(index)->preset_item();

            if (!Biz::Preset::PresetSelectionCheck::can_select_printer_preset(
                    preset_interactor,
                    item.hw_printer_config_id,
                    item.id
                ))
            {
                return;
            }

            preset_interactor.select_printer_preset(item.hw_printer_config_id, item.id);
            m_navigator.set_opened_dialog(nullptr);
            m_project_interactor.undo_provider().take_snapshot(
                UndoSnapshotType::SelectPrinterPreset
            );
        },
        [this](size_t index)
        {
            auto& preset_interactor = m_project_interactor.preset_interactor();
            const Biz::Preset::PresetItem& item =
                m_printer_list_view->item_at(index)->preset_item();

            auto selected_preset = preset_interactor.selected_printer_preset();
            if (selected_preset.hw_config.id == item.hw_printer_config_id
                && selected_preset.printer.id == item.id)
            {
                select_page_settings();
                return;
            }

            if (!Biz::Preset::PresetSelectionCheck::can_select_printer_preset(
                    preset_interactor,
                    item.hw_printer_config_id,
                    item.id
                ))
            {
                return;
            }

            select_page_settings();
            preset_interactor.select_printer_preset(item.hw_printer_config_id, item.id);
            m_project_interactor.undo_provider().take_snapshot(
                UndoSnapshotType::SelectPrinterPreset
            );
        },
        [this](size_t index) { m_preset_favorite_filter->invalidate(); },
        m_project_interactor.preset_interactor()
    );
    m_printer_list_view = scroll_area->emplace_back<PrinterListView>(std::move(factory));
    m_printer_list_view->set_flex_grow(1);
    m_printer_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_printer_list_view->set_margin(Margins(0, 0, -10, 0));
    m_printer_list_view->set_gap(8);
    m_printer_list_view->set_orientation(Orientation::Vertical);
    m_printer_list_view->set_source_list(m_preset_searcher.get());
    m_preset_searcher->set_source_model(m_preset_favorite_filter.get());
    m_preset_favorite_filter->set_source_model(
        &m_project_interactor.preset_interactor().printer_presets().items()
    );

    m_page_list->emplace_back<Separator>(Orientation::Horizontal);

    Item* row = m_page_list->emplace_back<Item>();
    row->set_justify_content(YGJustifySpaceBetween);

    LayoutButton* show_diff_dialog       = row->emplace_back<LayoutButton>(_u8L("Compare presets"));
    show_diff_dialog->callbacks().action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(m_project_interactor.preset_interactor());
    };
    /*
        LayoutButton* add_printer_button = row->emplace_back<LayoutButton>(_u8L("Add logical printer"));
        //    add_printer_button->set_self_align(YGAlignFlexEnd);
        add_printer_button->callbacks().action = [this] {
            m_printer_add_dialog->attach_to_item(content_item(), Position::Left);
            m_printer_add_dialog->set_current_tab(0);
            m_printer_add_dialog->open();
        };*/
}

void LogicalPrinterSettingsDialog::create_page_settings()
{
    m_page_settings = m_stack_layout->emplace_back<Item>();
    m_page_settings->set_orientation(Orientation::Vertical);
    m_page_settings->set_gap(5);
    m_page_settings->set_padding(10);

    Item* title_row = m_page_settings->emplace_back<Item>();
    title_row->set_align_items(YGAlignCenter);
    LayoutButton* back_button = title_row->emplace_back<LayoutButton>("", Render::Icon::CaretLeft);
    back_button->set_min_size({24.f, 24.f});
    back_button->set_content_padding(3.f);
    back_button->callbacks().action = [this]()
    {
        m_stack_layout->set_current_index(0);
        m_warning->set_visible(false);
    };
    m_text_printer_name = title_row->emplace_back<Text>("Unknown");

    m_page_settings->emplace_back<Separator>(Orientation::Horizontal);

    m_printer_icon = m_page_settings->emplace_back<Icon>(Render::Icon::PrinterNEXT);
    m_printer_icon->set_height(225);
    m_printer_icon->set_margin({0, 5});
    m_printer_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);

    m_page_settings->emplace_back<Text>(_u8L("Sheet"), Render::ImguiFontType::Bold);
    m_combo_sheets =
        m_page_settings
            ->emplace_back<Yoga::ComboBoxListViewSelection<Domain::Preset::HwSheetConfigDef>>();
    m_combo_sheets->set_get_name_fn(
        [](const Domain::Preset::HwSheetConfigDef* data) -> std::string { return data->name; }
    );
    m_combo_sheets->set_source_list(&m_project_interactor.preset_interactor().sheet_items());
    m_combo_sheets->callbacks().selection_changed = [this](int sheet_index)
    {
        if (sheet_index >= 0) {
            m_project_interactor.preset_interactor().select_printer_sheet(
                m_project_interactor.preset_interactor().sheet_items().items().at(sheet_index).id
            );
        }
    };

    Text* label = m_page_settings->emplace_back<Text>(_u8L("Nozzles"), Render::ImguiFontType::Bold);
    label->set_margin(Margins(0, 10, 0, 0));

    auto validation_updated = [this](bool valid) { m_warning->set_visible(!valid); };
    m_nozzle_list_view =
        m_page_settings->emplace_back<NozzleListView>(NozzleListView::ViewFactoryType{
            m_project_interactor.preset_interactor(),
            validation_updated
        });
    m_nozzle_list_view->set_orientation(Orientation::Vertical);
    m_nozzle_list_view->set_gap(5);
    m_nozzle_list_view->set_source_list(&m_project_interactor.preset_interactor().tool_items());

    m_warning = m_page_settings->emplace_back<Text>(
        _u8L("Invalid configuration"),
        Render::ImguiFontType::Bold
    );
    m_warning->set_visible(false);
    m_warning->set_text_color({0.98f, 0.4f, 0.19f});

    m_advanced_dialog->attach_to_item(content_item(), Position::Left);

    LayoutButton* button_advanced_setting =
        m_page_settings->emplace_back<LayoutButton>(_u8L("Advanced settings"), Render::Icon::Cog);
    button_advanced_setting->set_content_padding({0.f, 7.f});
    button_advanced_setting->set_height(30.f);
    button_advanced_setting->callbacks().action = [this]
    {
        if (m_advanced_dialog->opened()) {
            m_navigator.set_opened_dialog(this);
        } else {
            m_navigator.set_opened_dialog(m_advanced_dialog);
        }
    };
    m_advanced_dialog->callbacks().opened = [button_advanced_setting]
    { button_advanced_setting->set_checked(true); };
    m_advanced_dialog->callbacks().closed = [button_advanced_setting]
    { button_advanced_setting->set_checked(false); };
}

void LogicalPrinterSettingsDialog::on_about_to_show()
{
    m_stack_layout->set_current_index(0);
    m_warning->set_visible(false);
}

void LogicalPrinterSettingsDialog::update_settings_data()
{
    const Domain::Preset::HwPrinterConfig& printer_config =
        m_project_interactor.preset_interactor().current_printer_config();

    m_text_printer_name->set_text(printer_config.name);

    if (printer_config.visual.thumbnail.has_value()) {
        const std::string image_path =
            printer_config.relative_path_to_assets() + printer_config.visual.thumbnail.value();

        m_printer_icon->set_image(image_path);
    }
}

void LogicalPrinterSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

} // namespace Slic3r::App
