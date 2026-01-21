///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Biz/Preset/NameValidator.hpp"

#include <wx/dialog.h>

#include <map>

class wxString;
class wxStaticText;
class wxTextCtrl;
class wxStaticBitmap;
class wxCheckBox;
class wxComboBox;

namespace Slic3r::Biz::Preset {
class IPresetNameProvider;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::WX {

class SavePresetDialog : public wxDialog
{
public:
    using PresetKind     = Domain::Preset::PresetKind;
    using ValidationType = Biz::Preset::NameValidator::ValidationType;

    struct Item
    {
        Item(
            PresetKind kind,
            const std::string& name,
            const std::string& suffix,
            wxBoxSizer* sizer,
            SavePresetDialog* parent,
            bool is_for_multiple_save
        );
        Item(
            wxWindow* parent,
            wxBoxSizer* sizer,
            const std::string& def_name,
            Domain::PrinterTechnology pt = Domain::PrinterTechnology::FFF
        );

        void update_valid_bmp();
        void Enable(bool enable = true);

        bool is_valid() const
        {
            return m_valid_type != ValidationType::Invalid;
        }

        PresetKind kind() const
        {
            return m_kind;
        }

        std::string preset_name() const;

    private:
        void init_input_name_ctrl(wxBoxSizer* input_name_sizer, const std::string& preset_name);
        void update_state();

    private:
        PresetKind m_kind;
        Biz::Preset::NameValidator m_validator;
        bool m_use_text_ctrl{true};

        std::string m_preset_name;

        Domain::PrinterTechnology m_printer_technology{Domain::PrinterTechnology::FFF};
        ValidationType m_valid_type{ValidationType::Invalid};
        wxWindow* m_parent{nullptr};
        wxStaticBitmap* m_valid_bmp{nullptr};
        wxComboBox* m_combo{nullptr};
        wxTextCtrl* m_text_ctrl{nullptr};
        wxStaticText* m_valid_label{nullptr};
    };

public:
    SavePresetDialog(
        wxWindow* parent,
        std::map<PresetKind, std::string> names_per_kinds,
        const Biz::Preset::IPresetNameProvider& preset_interactor,
        std::string suffix     = "",
        bool template_filament = false
    );
    SavePresetDialog(
        wxWindow* parent,
        PresetKind kind,
        const std::string& name,
        const Biz::Preset::IPresetNameProvider& preset_interactor,
        const std::string& info_line_extension
    );
    ~SavePresetDialog() override;

    void AddItem(
        PresetKind kind,
        const std::string& name,
        const std::string& suffix,
        bool is_for_multiple_save
    );

    const Biz::Preset::IPresetNameProvider& preset_interactor() const;

    std::string get_name() const;
    std::string get_name(PresetKind kind) const;

    bool enable_ok_btn() const;
    bool Layout() override;

    bool is_for_rename() const;

    bool get_template_filament_checkbox() const;
    const wxString& get_info_line_extension() const;

private:
    void build(
        const std::map<PresetKind, std::string>& names_per_kinds,
        std::string suffix     = "",
        bool template_filament = false
    );
    void accept();

private:
    std::vector<std::unique_ptr<Item>> m_items;

    wxBoxSizer* m_presets_sizer{nullptr};
    wxStaticText* m_label{nullptr};
    wxBoxSizer* m_radio_sizer{nullptr};
    wxCheckBox* m_template_filament_checkbox{nullptr};

    std::string m_ph_printer_name;
    std::string m_old_preset_name;
    bool m_use_for_rename{false};
    wxString m_info_line_extension{wxEmptyString};
    const Biz::Preset::IPresetNameProvider& m_preset_interactor;
};
} // namespace Slic3r::App::WX
