///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/WX/UnsavedChangesDialog.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/App/Config/CategoryUtils.hpp"

#include "Slic3r/Domain/Preset/Bundle.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/FullConfigSLA.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/DiffViewCtrl.hpp"
#include "Slic3r/App/WX/DiffDVCModel.hpp"
#include "Slic3r/App/WX/DiffNamespace.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/Scalable.hpp"
#include "Slic3r/App/WX/I18N.hpp"

#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <wx/scrolwin.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace Slic3r::Biz;

namespace Slic3r::App::WX {

UnsavedChangesDialog::UnsavedChangesDialog(
    const std::string& dialog_name,
    const Domain::ConfigPack& config_original,
    const Domain::ConfigPack& config_selected,
    Domain::ConfigPack* config_new_selected,
    const Slic3r::Biz::Preset::PresetSelectionNames& preset_names,
    const Slic3r::Biz::Preset::PresetSelectionNames& preset_names_new
) :
    wxDialog(
        wxTheApp->GetTopWindow(),
        wxID_ANY,
        from_u8(dialog_name),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
    ),
    m_config_selected(config_selected),
    m_config_original(config_original),
    m_config_new(config_new_selected),
    m_preset_names(preset_names),
    m_preset_names_new(preset_names_new)
{
    if (std::get_if<Domain::ConfigPackFDM>(&m_config_original) == nullptr) {
        m_printer_technology = Domain::PrinterTechnology::SLA;
    }

    const wxFont font = GetFont().Bold();

    m_top_info_line = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_top_info_line->SetFont(font);

    m_bottom_info_line = new wxStaticText(this, wxID_ANY, from_u8("info from selection"));
    m_bottom_info_line->SetFont(font);

    int border             = w_config()->em_unit();
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    create_tree();

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    add_buttons(buttons);

    main_sizer->Add(m_top_info_line, 0, wxEXPAND | wxALL, border);
    main_sizer->Add(m_tree, 1, wxEXPAND | wxALL, border);
    main_sizer->Add(m_bottom_info_line, 0, wxEXPAND | wxALL, border);
    main_sizer->Add(buttons, 0, wxEXPAND | wxALL, border);

    this->SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);

    this->SetMinSize(wxSize(30 * w_config()->em_unit(), 50 * w_config()->em_unit()));
    this->CenterOnParent();
    w_config()->UpdateDlgDarkUI(this);

    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& evt) { m_tree->model->Rescale(); });

    compare();

    m_exit_queue = m_diffs_per_kind.size();
    show_current_diffs();
}

void UnsavedChangesDialog::create_tree()
{
    int em = w_config()->em_unit();
    m_tree = new DiffViewCtrl(this, wxSize(em * (m_config_new ? 80 : 65), em * 40));
    m_tree->SetFont(this->GetFont());

#ifdef __linux__
    const int toggle_column_width = 9;
#else
    const int toggle_column_width = 6;
#endif

    m_tree->AppendToggleColumn_(wxString(L"\u2714"), DiffDVCModel::colToggle, toggle_column_width);
    m_tree->AppendBmpTextColumn(wxEmptyString, DiffDVCModel::colIconText, 35);
    m_tree->AppendBmpTextColumn(_L("Original Value"), DiffDVCModel::colOldValue, 15);
    m_tree->AppendBmpTextColumn(_L("Modified Value"), DiffDVCModel::colModValue, 15);
    if (m_config_new)
        m_tree->AppendBmpTextColumn(_L("New Value"), DiffDVCModel::colNewValue, 15);
}

void UnsavedChangesDialog::add_buttons(wxBoxSizer* buttons)
{
    // Add Buttons
    wxFont btn_font = this->GetFont().Scaled(1.4f);

    auto add_btn = [this, buttons, btn_font](
                       ScalableButton** btn,
                       int& btn_id,
                       const std::string& icon_name,
                       Biz::Preset::PresetDiffOperation close_act,
                       const wxString& label,
                       bool process_enable = true
                   )
    {
        *btn = new ScalableButton(
            this,
            btn_id = NewControlId(),
            icon_name,
            label,
            wxDefaultSize,
            wxDefaultPosition,
            wxBORDER_DEFAULT,
            24
        );

        buttons->Add(*btn, 1, wxLEFT, 5);
        (*btn)->SetFont(btn_font);

        (*btn)->Bind(
            wxEVT_BUTTON,
            [this, close_act](wxEvent&) { process_button_click(close_act); }
        );
        if (process_enable)
            (*btn)->Bind(
                wxEVT_UPDATE_UI,
                [this](wxUpdateUIEvent& evt) { evt.Enable(m_tree->has_selection()); }
            );
        (*btn)->Bind(
            wxEVT_LEAVE_WINDOW,
            [this](wxMouseEvent& e)
            {
                show_info_line(Biz::Preset::PresetDiffOperation::Undef);
                e.Skip();
            }
        );

        (*btn)->Bind(
            wxEVT_ENTER_WINDOW,
            [this, close_act](wxMouseEvent& e)
            {
                show_info_line(close_act);
                e.Skip();
            }
        );
    };

    m_back_btn = new ScalableButton(
        this,
        wxID_ANY,
        "chevron_left",
        _L("Back"),
        wxDefaultSize,
        wxDefaultPosition,
        wxBORDER_DEFAULT,
        24
    );
    buttons->Add(m_back_btn, 1, wxLEFT, 5);
    m_back_btn->SetFont(btn_font);
    m_back_btn->Bind(
        wxEVT_BUTTON,
        [this](wxEvent&)
        {
            m_exit_queue++;
            show_current_diffs();
        }
    );

    // "Transfer" / "Keep" button
    // if (ActionButtons::TRANSFER & m_buttons)
    if (m_config_new) {
        // const PresetCollection*
        // switched_presets = type == Preset::TYPE_INVALID ? nullptr : wxGetApp().get_tab(type)->get_presets();
        // if (dependent_presets
        // && switched_presets
        // && (type == dependent_presets->type() ?
        // dependent_presets->get_edited_preset().printer_technology()
        // == dependent_presets->find_preset(new_selected_preset)->printer_technology() :
        // switched_presets->get_edited_preset().printer_technology()
        // == switched_presets->find_preset(new_selected_preset)->printer_technology()))
        add_btn(
            &m_transfer_btn,
            m_transfer_btn_id,
            "paste_menu",
            Biz::Preset::PresetDiffOperation::Transfer,
            /*switched_presets->get_edited_preset().name == new_selected_preset ? _L("Keep") : */
            _L("Transfer")
        );
    }
    // if (!m_transfer_btn && (ActionButtons::KEEP & m_buttons))
    // add_btn(&m_transfer_btn, m_transfer_btn_id, "paste_menu", Biz::Preset::PresetDiffOperation::Transfer, _L("Keep"));

    { // "Don't save" / "Discard" button
        std::string btn_icon = /*(ActionButtons::DONT_SAVE & m_buttons) ?
            "" :
            (dependent_presets || (ActionButtons::KEEP & m_buttons)) ?
            "switch_presets" :*/
            "exit";
        wxString btn_label =
            /* (ActionButtons::DONT_SAVE & m_buttons) ? _L("Don't save") : */ _L("Discard");
        add_btn(
            &m_discard_btn,
            m_continue_btn_id,
            btn_icon,
            Biz::Preset::PresetDiffOperation::Discard,
            btn_label,
            false
        );
    }

    // "Save" button
    // if (ActionButtons::SAVE & m_buttons)
    add_btn(
        &m_save_btn,
        m_save_btn_id,
        "save",
        Biz::Preset::PresetDiffOperation::Save,
        _L("Save"),
        false // Temporary: until save functionality is implemented
    );
    m_save_btn->Enable(false);// Temporary: until save functionality is implemented

    ScalableButton* cancel_btn = new ScalableButton(
        this,
        wxID_CANCEL,
        "cross",
        _L("Cancel"),
        wxDefaultSize,
        wxDefaultPosition,
        wxBORDER_DEFAULT,
        24
    );
    buttons->Add(cancel_btn, 1, wxLEFT | wxRIGHT, 5);
    cancel_btn->SetFont(btn_font);
    cancel_btn->Bind(
        wxEVT_BUTTON,
        [this](wxEvent&) { process_button_click(Biz::Preset::PresetDiffOperation::Undef); }
    );
}

void UnsavedChangesDialog::compare()
{
    m_diffs_per_kind.clear();
    std::vector<std::string> diff_keys;

    if (m_printer_technology == Domain::PrinterTechnology::FFF) {
        Domain::FullConfigFDM full_config_init(std::get<Domain::ConfigPackFDM>(m_config_original));
        Domain::FullConfigFDM full_config_selected(
            std::get<Domain::ConfigPackFDM>(m_config_selected)
        );

        diff_keys = full_config_init.diff_keys(full_config_selected);

        SPDLOG_INFO("UnChDlg: Diffs count: {} ", diff_keys.size());

        // distribute diffs per type
        Domain::ConfigPackFDM config = std::get<Domain::ConfigPackFDM>(m_config_original);
        for (const std::string& key : diff_keys) {
            if (config.printer.find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::FdmPrinter].emplace_back(key);
            } else if (config.print.find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::FdmPrint].emplace_back(key);
            } else if (config.tool[0].find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::FdmToolPrint].emplace_back(key);
            } else if (config.filament[0].find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::FdmMaterial].emplace_back(key);
            }
        }
    } else {
        Domain::ConfigPackSLA config = std::get<Domain::ConfigPackSLA>(m_config_original);
        Domain::FullConfigSLA full_config_init(std::get<Domain::ConfigPackSLA>(m_config_original));
        Domain::FullConfigSLA full_config_selected(
            std::get<Domain::ConfigPackSLA>(m_config_selected)
        );

        diff_keys = full_config_init.diff_keys(full_config_selected);

        // distribute diffs per type
        for (const std::string& key : diff_keys) {
            if (config.sla_printer_settings.find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::SlaPrinter].emplace_back(key);
            } else if (config.sla_print_settings.find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::SlaPrint].emplace_back(key);
            } else if (config.sla_material_settings.find(key).item) {
                m_diffs_per_kind[Domain::Preset::PresetKind::SlaMaterial].emplace_back(key);
            }
        }
    }
}

void UnsavedChangesDialog::show_current_diffs()
{
    size_t diffs_cnt = m_diffs_per_kind.size();
    size_t step      = diffs_cnt - m_exit_queue + 1;

    wxString info;
    if (!m_config_new)
        info += _L("Printer technology will be changed.") + from_u8("\n");
    info +=
        _L("Presets below have unsaved modifications.\n"
           "You need to process those modifications first.");

    if (diffs_cnt > 1) {
        const std::string suffix =
            fmt::vformat(_u8L("Step {} from {}"), fmt::make_format_args(step, diffs_cnt));
        info += from_u8("\n\n" + suffix);
    }
    m_top_info_line->SetLabel(info);

    auto it         = std::next(m_diffs_per_kind.begin(), step - 1);
    PresetKind kind = it->first;
    update_tree(kind, it->second);

    size_t tool_id = 0;
    update_transfer_button(kind, tool_id);

    m_back_btn->Show(diffs_cnt > 1);
    m_back_btn->Enable(step > 1);

    show_info_line(PresetDiffOperation::Undef);
}

void UnsavedChangesDialog::update_transfer_button(PresetKind kind, size_t tool_id)
{
    if (!m_transfer_btn)
        return;

    bool is_keep{false};
    switch (kind) {
    case PresetKind::FdmPrinter:
    case PresetKind::SlaPrinter: {
        is_keep = m_preset_names.printer == m_preset_names_new.printer;
        break;
    }
    case PresetKind::FdmPrint:
    case PresetKind::SlaPrint: {
        is_keep = m_preset_names.print == m_preset_names_new.print;
        break;
    }
    case PresetKind::FdmToolPrint: {
        is_keep = m_preset_names.tools[tool_id] == m_preset_names_new.tools[tool_id];
        break;
    }
    case PresetKind::FdmMaterial:
    case PresetKind::SlaMaterial: {
        is_keep = m_preset_names.materials[tool_id] == m_preset_names_new.materials[tool_id];
        break;
    }
    default:
        break;
    };

    m_transfer_btn->SetLabel(is_keep ? _L("Keep") : _L("Transfer"));
}

void UnsavedChangesDialog::append_diff_keys(
    Domain::Preset::PresetKind kind,
    const std::string& preset_name,
    const std::string& new_preset_name,
    const Domain::ConfigBox* config_left,
    const Domain::ConfigBox* config_mid,
    const Domain::ConfigBox* config_right,
    const std::vector<std::string>& diff_keys
)
{
    if (preset_name.empty() || diff_keys.empty() || !config_left || !config_mid) {
        return;
    }

    m_tree->model->AddPreset(kind, preset_name, new_preset_name);

    for (const std::string& key : diff_keys) {
        const Domain::ConfigItem& item_left = *config_left->find(key).item;
        const Domain::ConfigItem& item_mid  = *config_mid->find(key).item;

        if (item_left == item_mid)
            continue;

        std::string left_val  = Diff::get_as_string(item_left);
        std::string mid_val   = Diff::get_as_string(item_mid);
        std::string right_val = "";
        if (config_right) {
            const Domain::ConfigItem& item_right = *config_right->find(key).item;
            right_val                            = Diff::get_as_string(item_right);
        }

        const Domain::ConfigItemDef& def = item_left.def();
        m_tree->Append(
            key,
            kind,
            preset_name,
            Domain::ConfigItemDef::translate_category(def.category, m_printer_technology),
            def.option_group.empty() ? def.label : def.option_group,
            def.full_label.empty() ? def.label : def.full_label,
            left_val,
            mid_val,
            right_val,
            CategoryUtils::category_icon_name(def.category, m_printer_technology)
        );
    }
}

static std::string name(Slic3r::Biz::Preset::PresetSelectionNames::PresetName preset_name)
{
    const std::string prefix{preset_name.is_runtime_only ? _u8L("(From 3mf) ") : ""};
    return prefix + preset_name.name;
}

void UnsavedChangesDialog::update_tree(PresetKind kind, const std::vector<std::string>& diff_keys)
{
    m_tree->Clear();

    const Domain::ConfigBox* config_left{nullptr};
    const Domain::ConfigBox* config_mid{nullptr};
    const Domain::ConfigBox* config_right{nullptr};
    std::string preset_name;

    switch (kind) {
    case PresetKind::FdmPrinter:
    case PresetKind::SlaPrinter: {
        preset_name = fmt::format("{} \"{}\"", _u8L("Printer"), name(m_preset_names.printer));
        if (kind == PresetKind::FdmPrinter) {
            config_left = &std::get<Domain::ConfigPackFDM>(m_config_original).printer;
            config_mid  = &std::get<Domain::ConfigPackFDM>(m_config_selected).printer;
            if (m_config_new) {
                config_right = &std::get<Domain::ConfigPackFDM>(*m_config_new).printer;
            }
        } else {
            config_left = &std::get<Domain::ConfigPackSLA>(m_config_original).sla_printer_settings;
            config_mid  = &std::get<Domain::ConfigPackSLA>(m_config_selected).sla_printer_settings;
            if (m_config_new) {
                config_right = &std::get<Domain::ConfigPackSLA>(*m_config_new).sla_printer_settings;
            }
        }
        break;
    }
    case PresetKind::FdmPrint:
    case PresetKind::SlaPrint: {
        preset_name = fmt::format("{} \"{}\"", _u8L("Print"), name(m_preset_names.print));
        if (kind == PresetKind::FdmPrint) {
            config_left = &std::get<Domain::ConfigPackFDM>(m_config_original).print;
            config_mid  = &std::get<Domain::ConfigPackFDM>(m_config_selected).print;
            if (m_config_new) {
                config_right = &std::get<Domain::ConfigPackFDM>(*m_config_new).print;
            }
        } else {
            config_left = &std::get<Domain::ConfigPackSLA>(m_config_original).sla_print_settings;
            config_mid  = &std::get<Domain::ConfigPackSLA>(m_config_selected).sla_print_settings;
            if (m_config_new) {
                config_right = &std::get<Domain::ConfigPackSLA>(*m_config_new).sla_print_settings;
            }
        }
        break;
    }
    case PresetKind::FdmToolPrint: {
        for (size_t i = 0, n = m_preset_names.tools.size(); i < n; i++) {
            config_left = &std::get<Domain::ConfigPackFDM>(m_config_original).tool[i];
            config_mid  = &std::get<Domain::ConfigPackFDM>(m_config_selected).tool[i];
            if (m_config_new && std::get<Domain::ConfigPackFDM>(*m_config_new).tool.size() > i) {
                config_right = &std::get<Domain::ConfigPackFDM>(*m_config_new).tool[i];
            } else {
                config_right = nullptr;
            }

            bool equal = true;
            for (const std::string& key : diff_keys) {
                const Domain::ConfigItem& item_left = *config_left->find(key).item;
                const Domain::ConfigItem& item_mid  = *config_mid->find(key).item;
                if (item_left != item_mid) {
                    equal = false;
                    break;
                }
            }
            if (equal)
                continue;

            preset_name =
                fmt::format("{} {} \"{}\"", _u8L("Tool Print"), i, name(m_preset_names.tools[i]));
            append_diff_keys(
                kind,
                preset_name,
                m_preset_names_new.tools.empty() ? "" : name(m_preset_names_new.tools[i]),
                config_left,
                config_mid,
                config_right,
                diff_keys
            );
            preset_name.clear();
        }
        break;
    }
    case PresetKind::FdmMaterial: {
        for (size_t i = 0, n = m_preset_names.materials.size(); i < n; i++) {
            config_left = &std::get<Domain::ConfigPackFDM>(m_config_original).filament[i];
            config_mid  = &std::get<Domain::ConfigPackFDM>(m_config_selected).filament[i];
            if (m_config_new && std::get<Domain::ConfigPackFDM>(*m_config_new).filament.size() > i)
            {
                config_right = &std::get<Domain::ConfigPackFDM>(*m_config_new).filament[i];
            } else {
                config_right = nullptr;
            }

            bool equal = true;
            for (const std::string& key : diff_keys) {
                const Domain::ConfigItem& item_left = *config_left->find(key).item;
                const Domain::ConfigItem& item_mid  = *config_mid->find(key).item;
                if (item_left != item_mid) {
                    equal = false;
                    break;
                }
            }
            if (equal)
                continue;

            preset_name = fmt::format(
                "{} {} \"{}\"",
                _u8L("Tool Filament"),
                i,
                name(m_preset_names.materials[i])
            );
            append_diff_keys(
                kind,
                preset_name,
                m_preset_names_new.materials.empty() ? "" : name(m_preset_names_new.materials[i]),
                config_left,
                config_mid,
                config_right,
                diff_keys
            );
            preset_name.clear();
        }
        break;
    }
    case PresetKind::SlaMaterial: {
        preset_name = fmt::format("{} \"{}\"", _u8L("Material"), name(m_preset_names.materials[0]));
        config_left = &std::get<Domain::ConfigPackSLA>(m_config_original).sla_material_settings;
        config_mid  = &std::get<Domain::ConfigPackSLA>(m_config_selected).sla_material_settings;
        if (m_config_new) {
            config_right = &std::get<Domain::ConfigPackSLA>(*m_config_new).sla_material_settings;
        }
        break;
    }
    default:
        break;
    };

    if (preset_name.empty()) {
        return;
    }

    std::string new_preset_name = kind == PresetKind::SlaMaterial ?
        name(m_preset_names_new.materials[0]) :
        kind == PresetKind::FdmPrint || kind == PresetKind::SlaPrint ?
        name(m_preset_names_new.print) :
        name(m_preset_names_new.printer);

    append_diff_keys(
        kind,
        preset_name,
        new_preset_name,
        config_left,
        config_mid,
        config_right,
        diff_keys
    );
}

void UnsavedChangesDialog::show_info_line(
    Biz::Preset::PresetDiffOperation operation,
    std::string preset_name
)
{
    if (operation == Biz::Preset::PresetDiffOperation::Undef && !m_tree->has_long_strings())
        m_bottom_info_line->Hide();
    else {
        wxString text;
        if (operation == Biz::Preset::PresetDiffOperation::Undef)
            text = _L("Some fields are too long to fit. Right mouse click reveals the full text.");
        else if (operation == Biz::Preset::PresetDiffOperation::Discard)
            text = _L("All settings changes will be discarded.");
        else {
            if (preset_name.empty())
                text = operation == Biz::Preset::PresetDiffOperation::Save ?
                    _L("Save the selected options.") :
                    _L("Keep the selected settings.");
            //_L("Transfer the selected settings to the newly selected preset.");
            else
                text = from_u8(
                    fmt::vformat(
                        operation == Biz::Preset::PresetDiffOperation::Save ?
                            _u8L("Save the selected options to preset \"%1%\".") :
                            _u8L(
                                "Transfer the selected options to the newly selected preset \"%1%\"."
                            ),
                        fmt::make_format_args(preset_name)
                    )
                );
            text += from_u8("\n") + _L("Unselected options will be reverted.");
        }
        m_bottom_info_line->SetLabel(text);
        m_bottom_info_line->Show();
    }

    Layout();
    Refresh();
}

void UnsavedChangesDialog::process_button_click(PresetDiffOperation operation)
{
    if (operation == PresetDiffOperation::Undef) {
        m_exit_states.clear();
        this->EndModal(wxID_CLOSE);
        return;
    }

    const size_t step = m_diffs_per_kind.size() - m_exit_queue;
    PresetKind kind   = std::next(m_diffs_per_kind.begin(), step)->first;

    size_t tool_id = 0; // ToDo: detect tool/material id

    Slic3r::Biz::Preset::PresetSwitchKindId id = {kind};
    if (kind == PresetKind::FdmToolPrint || kind == PresetKind::FdmMaterial) {
        id.id = tool_id;
    }
    m_exit_states[id] = {operation, {}, {}};

    if (operation == PresetDiffOperation::Discard) {
        // ignore this state
    } else {
        if (operation == PresetDiffOperation::Save) {
            std::string new_preset_name = "TEMP new name";
            // ToDo: get new preset name using SavePresetDialog

            m_exit_states[id].new_preset_name = new_preset_name;
        }

        const Domain::ConfigPack& config_pack =
            operation == PresetDiffOperation::Save ? m_config_original : m_config_selected;

        std::vector<std::string> options = operation == PresetDiffOperation::Save ?
            m_tree->unselected_options() :
            m_tree->selected_options();

        if (m_printer_technology == Domain::PrinterTechnology::FFF) {
            Domain::ConfigPackFDM config = std::get<Domain::ConfigPackFDM>(config_pack);

            const Domain::ConfigItems& items = //
                kind == PresetKind::FdmPrinter   ? config.printer.items :
                kind == PresetKind::FdmPrint     ? config.print.items :
                kind == PresetKind::FdmToolPrint ? config.tool[tool_id].items :
                                                   config.filament[tool_id].items;

            const Domain::ConfigOverrides& overrides = //
                kind == PresetKind::FdmPrinter   ? config.printer.overrides :
                kind == PresetKind::FdmPrint     ? config.print.overrides :
                kind == PresetKind::FdmToolPrint ? config.tool[tool_id].overrides :
                                                   config.filament[tool_id].overrides;

            for (const std::string& key : options) {
                m_exit_states[id].items.insert({key, items.opt(key).value()});
                if (const Domain::ConfigItem* override = overrides.find(key)) {
                    m_exit_states[id].overrides.insert({key, override->value()});
                }
            }
        } else {
            Domain::ConfigPackSLA config = std::get<Domain::ConfigPackSLA>(config_pack);

            const Domain::ConfigItems& items = //
                kind == PresetKind::SlaPrinter ? config.sla_printer_settings.items :
                kind == PresetKind::SlaPrint   ? config.sla_print_settings.items :
                                                 config.sla_material_settings.items;

            const Domain::ConfigOverrides& overrides = //
                kind == PresetKind::SlaPrinter ? config.sla_printer_settings.overrides :
                kind == PresetKind::SlaPrint   ? config.sla_print_settings.overrides :
                                                 config.sla_material_settings.overrides;

            for (const std::string& key : options) {
                m_exit_states[id].items.insert({key, items.opt(key).value()});
                if (const Domain::ConfigItem* override = overrides.find(key)) {
                    m_exit_states[id].overrides.insert({key, override->value()});
                }
            }
        }
    }

    m_exit_queue--;
    if (m_exit_queue == 0) {
        this->EndModal(wxID_CLOSE);
    } else {
        show_current_diffs();
    }
}

} // namespace Slic3r::App::WX
