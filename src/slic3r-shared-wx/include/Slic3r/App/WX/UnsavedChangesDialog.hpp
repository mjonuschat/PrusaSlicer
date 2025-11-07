///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"
#include "Slic3r/Biz/Preset/PresetDiffOperation.hpp"
#include "Slic3r/Biz/Preset/PresetSelectionNames.hpp"
#include "Slic3r/Biz/Preset/IPresetDialogManager.hpp"

#include <wx/dialog.h>

class wxStaticText;
class wxBoxSizer;
namespace Slic3r::App::WX {
class DiffViewCtrl;
class ScalableButton;

class UnsavedChangesDialog : public wxDialog
{
public:
    using PresetKind          = Domain::Preset::PresetKind;
    using PresetDiffOperation = Biz::Preset::PresetDiffOperation;
    using PresetsSwitchStates = Biz::Preset::IPresetDialogManager::PresetsSwitchStates;

    UnsavedChangesDialog(
        const std::string& dialog_name,
        const Domain::ConfigPack& config_original,
        const Domain::ConfigPack& config_selected,
        Domain::ConfigPack* config_new_selected,
        const Biz::Preset::PresetSelectionNames& preset_names,
        const Biz::Preset::PresetSelectionNames& preset_names_new
    );
    ~UnsavedChangesDialog() = default;

    PresetsSwitchStates exit_states() const
    {
        return m_exit_states;
    }

private:
    void create_tree();
    void add_buttons(wxBoxSizer* sizer);
    void compare();
    void show_current_diffs();
    void update_transfer_button(PresetKind kind, size_t tool_id = 0);
    void update_tree(PresetKind kind, const std::vector<std::string>& diff_keys);
    void append_diff_keys(
        PresetKind kind,
        const std::string& preset_name,
        const std::string& new_preset_name,
        const Domain::ConfigBox* config_left,
        const Domain::ConfigBox* config_mid,
        const Domain::ConfigBox* config_right,
        const std::vector<std::string>& diff_keys
    );
    void show_info_line(PresetDiffOperation operation, std::string preset_name = "");

    void process_button_click(PresetDiffOperation operation);

private:
    wxStaticText* m_top_info_line{nullptr};
    wxStaticText* m_bottom_info_line{nullptr};

    DiffViewCtrl* m_tree{nullptr};

    ScalableButton* m_back_btn{nullptr};
    ScalableButton* m_save_btn{nullptr};
    ScalableButton* m_transfer_btn{nullptr};
    ScalableButton* m_discard_btn{nullptr};

    int m_save_btn_id{wxID_ANY};
    int m_transfer_btn_id{wxID_ANY};
    int m_continue_btn_id{wxID_ANY};

    Biz::Preset::PresetSelectionNames m_preset_names;
    Biz::Preset::PresetSelectionNames m_preset_names_new;
    Domain::PrinterTechnology m_printer_technology{Domain::PrinterTechnology::FFF};

    const Domain::ConfigPack m_config_original;
    const Domain::ConfigPack m_config_selected;
    Domain::ConfigPack* m_config_new{nullptr};

    using DiffsPerKind = std::map<PresetKind, std::vector<std::string>>;
    DiffsPerKind m_diffs_per_kind;

    PresetsSwitchStates m_exit_states;

    // Indicates a count of preset checkes in queue before close the dialog
    int m_exit_queue{0};
};

} // namespace Slic3r::App::WX
