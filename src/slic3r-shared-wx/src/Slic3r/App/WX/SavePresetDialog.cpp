///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/WX/SavePresetDialog.hpp"

#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/I18N.hpp"

#include <vector>
#include <memory>
#include <fmt/format.h>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>

namespace Slic3r::App::WX {

constexpr auto BORDER_W = 10;

//-----------------------------------------------
// SavePresetDialog::Item
//-----------------------------------------------

void SavePresetDialog::Item::init_input_name_ctrl(
    wxBoxSizer* input_name_sizer,
    const std::string& preset_name
)
{
    if (m_use_text_ctrl) {
#ifdef _WIN32
        long style = wxBORDER_SIMPLE;
#else
        long style = 0L;
#endif
        m_text_ctrl = new wxTextCtrl(
            m_parent,
            wxID_ANY,
            from_u8(preset_name),
            wxDefaultPosition,
            wxSize(35 * w_config()->em_unit(), -1),
            style
        );
        w_config()->UpdateDarkUI(m_text_ctrl);
        m_text_ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update_state(); });

        input_name_sizer->Add(m_text_ctrl, 1, wxEXPAND, BORDER_W);
    } else {
        m_combo = new wxComboBox(
            m_parent,
            wxID_ANY,
            wxEmptyString,
            wxDefaultPosition,
            wxSize(35 * w_config()->em_unit(), -1)
        );
        for (const auto& preset : as_const(m_validator.preset_names())) {
            if (preset.origin == Domain::Preset::PresetOrigin::User) {
                m_combo->Append(from_u8(preset.name));
            }
        }
        m_combo->SetValue(from_u8(preset_name));

        m_combo->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update_state(); });

        input_name_sizer->Add(m_combo, 1, wxEXPAND, BORDER_W);
    }
}

static const std::map<Domain::Preset::PresetKind, std::string> TOP_LABELS = {
    // kind                             Save settings
    {Domain::Preset::PresetKind::FdmPrinter, Biz::L("Save printer settings as")},
    {Domain::Preset::PresetKind::FdmPrint, Biz::L("Save print settings as")},
    {Domain::Preset::PresetKind::FdmToolPrint, Biz::L("Save tool print settings as")},
    {Domain::Preset::PresetKind::FdmMaterial, Biz::L("Save filament settings as")},
    {Domain::Preset::PresetKind::SlaPrinter, Biz::L("Save printer settings as")},
    {Domain::Preset::PresetKind::SlaPrint, Biz::L("Save print settings as")},
    {Domain::Preset::PresetKind::SlaToolPrint, Biz::L("Save tool settings as")},
    {Domain::Preset::PresetKind::SlaMaterial, Biz::L("Save material settings as")},
};

SavePresetDialog::Item::Item(
    PresetKind kind,
    const std::string& name,
    const std::string& suffix,
    wxBoxSizer* sizer,
    SavePresetDialog* parent,
    bool is_for_multiple_save
) :
    m_kind(kind),
    m_validator(parent->preset_interactor(), kind, name, parent->is_for_rename()),
    m_use_text_ctrl(parent->is_for_rename()),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, wxEmptyString))
{
    m_valid_label->SetFont(w_config()->bold_font());

    wxStaticText* label_top = is_for_multiple_save ?
        new wxStaticText(m_parent, wxID_ANY, _(TOP_LABELS.at(m_kind)) + from_u8(":")) :
        nullptr;

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);

    std::string init_name;
    for (const Domain::Preset::PresetName& preset_name : as_const(m_validator.preset_names())) {
        if (preset_name.name == name) {
            init_name = preset_name.origin == Domain::Preset::PresetOrigin::System ?
                fmt::format("{} - {}", name, suffix) :
                name;
            break;
        }
    }
    ASSERT(!init_name.empty());

    init_input_name_ctrl(input_name_sizer, init_name);

    if (label_top)
        sizer->Add(label_top, 0, wxEXPAND | wxTOP | wxBOTTOM, BORDER_W);
    sizer->Add(input_name_sizer, 0, wxEXPAND | (label_top ? 0 : wxTOP) | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label, 0, wxEXPAND | wxLEFT, 3 * BORDER_W);

    update_state();
}

SavePresetDialog::Item::Item(
    wxWindow* parent,
    wxBoxSizer* sizer,
    const std::string& def_name,
    Domain::PrinterTechnology pt /*= Domain::PrinterTechnology::FFF*/
) :
    m_preset_name(def_name),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, wxEmptyString))
{
    m_valid_label->SetFont(w_config()->bold_font());

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, m_preset_name);

    sizer->Add(input_name_sizer, 0, wxEXPAND | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label, 0, wxEXPAND | wxLEFT, 3 * BORDER_W);

    update_state();
}

std::string SavePresetDialog::Item::preset_name() const
{
    if (m_use_text_ctrl)
        return m_preset_name;

    const std::string existed_preset_name = m_validator.get_conflict_name(m_preset_name);
    if (existed_preset_name.empty())
        return m_preset_name;

    return existed_preset_name;
}

void SavePresetDialog::Item::update_state()
{
    m_preset_name = into_u8(m_use_text_ctrl ? m_text_ctrl->GetValue() : m_combo->GetValue());

    const auto validation_res = m_validator.validate(m_preset_name);

    m_valid_type       = validation_res.type;
    wxString info_line = from_u8(validation_res.message);

    // Parent is expected to be SavePresetDialog
    SavePresetDialog* dlg = dynamic_cast<SavePresetDialog*>(m_parent);
    if ((dlg && !dlg->get_info_line_extension().IsEmpty())
        && m_valid_type != ValidationType::Invalid)
        info_line += from_u8("\n\n") + dlg->get_info_line_extension();

    m_valid_label->SetLabel(info_line);
    m_valid_label->Show(!info_line.IsEmpty());

    update_valid_bmp();

    m_parent->Layout();
}

void SavePresetDialog::Item::update_valid_bmp()
{
    std::string bmp_name = m_valid_type == ValidationType::Warning ? "exclamation_manifold" :
        m_valid_type == ValidationType::Invalid                    ? "exclamation" :
                                                                     "tick_mark";
    m_valid_bmp->SetBitmap(*get_bmp_bundle(bmp_name));
}

void SavePresetDialog::Item::Enable(bool enable /*= true*/)
{
    m_valid_label->Enable(enable);
    m_valid_bmp->Enable(enable);
    m_use_text_ctrl ? m_text_ctrl->Enable(enable) : m_combo->Enable(enable);
}

//-----------------------------------------------
// SavePresetDialog
//-----------------------------------------------

SavePresetDialog::SavePresetDialog(
    wxWindow* parent,
    std::map<PresetKind, std::string> names_per_kinds,
    const Biz::Preset::IPresetNameProvider& preset_interactor,
    std::string suffix,
    bool template_filament /* =false*/
) :
    wxDialog(
        parent,
        wxID_ANY,
        names_per_kinds.size() == 1 ? _L("Save preset") : _L("Save presets"),
        wxDefaultPosition,
        wxSize(45 * w_config()->em_unit(), 5 * w_config()->em_unit()),
        wxDEFAULT_DIALOG_STYLE | wxICON_WARNING
    ),
    m_preset_interactor(preset_interactor)
{
    build(names_per_kinds, suffix, template_filament);
}

SavePresetDialog::SavePresetDialog(
    wxWindow* parent,
    PresetKind kind,
    const std::string& name,
    const Biz::Preset::IPresetNameProvider& preset_interactor,
    const std::string& info_line_extension
) :
    wxDialog(
        parent,
        wxID_ANY,
        _L("Rename preset"),
        wxDefaultPosition,
        wxSize(45 * w_config()->em_unit(), 5 * w_config()->em_unit()),
        wxDEFAULT_DIALOG_STYLE | wxICON_WARNING
    ),
    m_use_for_rename(true),
    m_info_line_extension(from_u8(info_line_extension)),
    m_preset_interactor(preset_interactor)
{
    build({{kind, name}});
}

SavePresetDialog::~SavePresetDialog() = default;

void SavePresetDialog::build(
    const std::map<PresetKind, std::string>& names_per_kinds,
    std::string suffix,
    bool template_filament
)
{
    this->SetFont(w_config()->normal_font());

    if (suffix.empty())
        // TRN Suffix for the preset name. Have to be a noun.
        suffix = Biz::_CTX_utf8(Biz::L_CONTEXT("Copy", "PresetName"), "PresetName");

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    m_presets_sizer = new wxBoxSizer(wxVERTICAL);

    const bool is_for_multiple_save = names_per_kinds.size() > 1;
    for (const auto& [kind, name] : names_per_kinds)
        AddItem(kind, name, suffix, is_for_multiple_save);

    // Add dialog's buttons
    wxStdDialogButtonSizer* btns = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    wxButton* btnOK              = static_cast<wxButton*>(this->FindWindowById(wxID_OK, this));
    btnOK->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { accept(); });
    btnOK->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& evt) { evt.Enable(enable_ok_btn()); });

    topSizer->Add(m_presets_sizer, 0, wxEXPAND | wxALL, BORDER_W);

    // Add checkbox for Template filament saving
    if (template_filament
        && names_per_kinds.size() == 1
        && names_per_kinds.begin()->first == PresetKind::FdmMaterial)
    {
        m_template_filament_checkbox = new wxCheckBox(
            this,
            wxID_ANY,
            _L("Save as profile derived from current printer only.")
        );
        wxBoxSizer* check_sizer = new wxBoxSizer(wxVERTICAL);
        check_sizer->Add(m_template_filament_checkbox);
        topSizer->Add(check_sizer, 0, wxEXPAND | wxALL, BORDER_W);
    }

    topSizer->Add(btns, 0, wxEXPAND | wxALL, BORDER_W);

    SetSizer(topSizer);
    topSizer->SetSizeHints(this);

    this->CenterOnScreen();

    w_config()->UpdateDlgDarkUI(this);
}

void SavePresetDialog::AddItem(
    PresetKind kind,
    const std::string& name,
    const std::string& suffix,
    bool is_for_multiple_save
)
{
    m_items.emplace_back(
        std::make_unique<Item>(kind, name, suffix, m_presets_sizer, this, is_for_multiple_save)
    );
}

const Biz::Preset::IPresetNameProvider& SavePresetDialog::preset_interactor() const
{
    return m_preset_interactor;
}

std::string SavePresetDialog::get_name() const
{
    return m_items.front()->preset_name();
}

std::string SavePresetDialog::get_name(PresetKind kind) const
{
    // All preset kinds passed to this dialog must have a corresponding Item.
    // Missing kind indicates a programming error.
    for (const auto& item : m_items)
        if (item->kind() == kind)
            return item->preset_name();
    PANIC("SavePresetDialog::get_name(): preset kind not present in dialog");
}

bool SavePresetDialog::get_template_filament_checkbox() const
{
    if (m_template_filament_checkbox) {
        return m_template_filament_checkbox->GetValue();
    }
    return false;
}

const wxString& App::WX::SavePresetDialog::get_info_line_extension() const
{
    return m_info_line_extension;
}

bool SavePresetDialog::enable_ok_btn() const
{
    for (const auto& item : m_items)
        if (!item->is_valid())
            return false;

    return true;
}

bool SavePresetDialog::Layout()
{
    const bool ret = wxDialog::Layout();
    this->Fit();
    return ret;
}

bool SavePresetDialog::is_for_rename() const
{
    return m_use_for_rename;
}

void SavePresetDialog::accept()
{
    EndModal(wxID_OK);
}

} // namespace Slic3r::App::WX
