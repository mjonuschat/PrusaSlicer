///|/ Copyright (c) Prusa Research 2019 - 2023 Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros
///|/ Copyright (c) OrcaSlicer 2023 Noisyfox @Noisyfox
///|/ Copyright (c) SuperSlicer 2020 Remi Durand @supermerill
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
// #include "libslic3r/GCodeSender.hpp"
#include "ConfigManipulation.hpp"
#include "I18N.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "MsgDialog.hpp"

#include <string>
#include <wx/msgdlg.h>

namespace Slic3r {
namespace GUI {

// These track widths that have been explicitly approved by the user to suppress validation warnings
static std::map<std::string, double> s_approved_narrow_widths; // below 60% of nozzle
static std::map<std::string, double> s_approved_wide_widths;   // above 150% of nozzle

// Flag to suppress extrusion width warnings during initial app load
static bool s_suppress_extrusion_width_warnings = false;

void ConfigManipulation::set_suppress_extrusion_width_warnings(bool suppress)
{
    s_suppress_extrusion_width_warnings = suppress;
}

void ConfigManipulation::approve_extrusion_width(const std::string &width_key, double width_mm)
{
    // Pre-approve both narrow and wide for this key at this value
    s_approved_narrow_widths[width_key] = width_mm;
    s_approved_wide_widths[width_key] = width_mm;
}

void ConfigManipulation::clear_approved_widths()
{
    s_approved_narrow_widths.clear();
    s_approved_wide_widths.clear();
}

void ConfigManipulation::apply(DynamicPrintConfig* config, DynamicPrintConfig* new_config)
{
    bool modified = false;
    for (auto opt_key : config->diff(*new_config)) {
        config->set_key_value(opt_key, new_config->option(opt_key)->clone());
        modified = true;
    }

    if (modified && load_config != nullptr)
        load_config();
}

void ConfigManipulation::toggle_field(const std::string& opt_key, const bool toggle, int opt_index/* = -1*/)
{
    if (local_config) {
        if (local_config->option(opt_key) == nullptr)
            return;
    }
    cb_toggle_field(opt_key, toggle, opt_index);
}

std::optional<DynamicPrintConfig> handle_automatic_extrusion_widths(const DynamicPrintConfig &config, const bool is_global_config, wxWindow *msg_dlg_parent)
{
    const std::vector<std::string> extrusion_width_parameters = {"extrusion_width", "external_perimeter_extrusion_width", "first_layer_extrusion_width",
                                                                 "infill_extrusion_width", "perimeter_extrusion_width", "solid_infill_extrusion_width",
                                                                 "support_material_extrusion_width", "top_infill_extrusion_width"};

    auto is_zero_width = [](const ConfigOptionFloatOrPercent &opt) -> bool {
        return opt.value == 0. && !opt.percent;
    };

    auto is_parameters_adjustment_needed = [&is_zero_width, &config, &extrusion_width_parameters]() -> bool {
        if (!config.opt_bool("automatic_extrusion_widths")) {
            return false;
        }

        for (const std::string &extrusion_width_parameter : extrusion_width_parameters) {
            if (!is_zero_width(*config.option<ConfigOptionFloatOrPercent>(extrusion_width_parameter))) {
                return true;
            }
        }

        return false;
    };

    if (is_parameters_adjustment_needed()) {
        wxString msg_text = _(L("The automatic extrusion widths calculation requires:\n"
                                "- Default extrusion width: 0\n"
                                "- First layer extrusion width: 0\n"
                                "- Perimeter extrusion width: 0\n"
                                "- External perimeter extrusion width: 0\n"
                                "- Infill extrusion width: 0\n"
                                "- Solid infill extrusion width: 0\n"
                                "- Top infill extrusion width: 0\n"
                                "- Support material extrusion width: 0"));

        if (is_global_config) {
            msg_text += "\n\n" + _(L("Shall I adjust those settings in order to enable automatic extrusion widths calculation?"));
        }

        MessageDialog dialog(msg_dlg_parent, msg_text, _(L("Automatic extrusion widths calculation")),
                                  wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK));

        const int          answer   = dialog.ShowModal();
        DynamicPrintConfig new_conf = config;
        if (!is_global_config || answer == wxID_YES) {
            for (const std::string &extrusion_width_parameter : extrusion_width_parameters) {
                new_conf.set_key_value(extrusion_width_parameter, new ConfigOptionFloatOrPercent(0., false));
            }
        } else {
            new_conf.set_key_value("automatic_extrusion_widths", new ConfigOptionBool(false));
        }

        return new_conf;
    }

    return std::nullopt;
}

void ConfigManipulation::update_print_fff_config(DynamicPrintConfig* config, const bool is_global_config,
                                 const std::string &changed_opt_key)
{
    // #ys_FIXME_to_delete
    //! Temporary workaround for the correct updates of the TextCtrl (like "layer_height"):
    // KillFocus() for the wxSpinCtrl use CallAfter function. So,
    // to except the duplicate call of the update() after dialog->ShowModal(),
    // let check if this process is already started.
    if (is_msg_dlg_already_exist)
        return;

    // Determine if we should validate extrusion widths
    // Only validate when the changed key is an extrusion width or nozzle_diameter
    static const std::set<std::string> extrusion_width_keys = {"extrusion_width",
                                                               "first_layer_extrusion_width",
                                                               "perimeter_extrusion_width",
                                                               "external_perimeter_extrusion_width",
                                                               "infill_extrusion_width",
                                                               "solid_infill_extrusion_width",
                                                               "top_infill_extrusion_width",
                                                               "support_material_extrusion_width",
                                                               "support_material_interface_extrusion_width",
                                                               "bridge_extrusion_width",
                                                               "nozzle_diameter"};
    bool should_validate_extrusion_widths = changed_opt_key.empty() || extrusion_width_keys.count(changed_opt_key) > 0;

    // layer_height shouldn't be equal to zero
    if (config->opt_float("layer_height") < EPSILON)
    {
        const wxString msg_text = _(L("Layer height is not valid.\n\nThe layer height will be reset to 0.01."));
        MessageDialog dialog(m_msg_dlg_parent, msg_text, _(L("Layer height")), wxICON_WARNING | wxOK);
        DynamicPrintConfig new_conf = *config;
        is_msg_dlg_already_exist = true;
        dialog.ShowModal();
        new_conf.set_key_value("layer_height", new ConfigOptionFloat(0.01));
        apply(config, &new_conf);
        is_msg_dlg_already_exist = false;
    }

    if (config->option<ConfigOptionFloatOrPercent>("first_layer_height")->value < EPSILON)
    {
        const wxString msg_text = _(L("First layer height is not valid.\n\nThe first layer height will be reset to 0.01."));
        MessageDialog dialog(m_msg_dlg_parent, msg_text, _(L("First layer height")), wxICON_WARNING | wxOK);
        DynamicPrintConfig new_conf = *config;
        is_msg_dlg_already_exist = true;
        dialog.ShowModal();
        new_conf.set_key_value("first_layer_height", new ConfigOptionFloatOrPercent(0.01, false));
        apply(config, &new_conf);
        is_msg_dlg_already_exist = false;
    }

    // Helper lambda to clamp overlap values and warn user (uses WarningDialog style)
    auto clamp_overlap = [&](const std::string &opt_key, const std::string &ref_width_key, double min_percent,
                             double max_percent, const std::string &label, const std::string &ref_width_label)
    {
        auto *overlap_opt = config->option<ConfigOptionFloatOrPercent>(opt_key);
        if (!overlap_opt)
            return;

        // Get reference width for mm clamping
        // Note: nozzle_diameter is in printer config, not print config
        const DynamicPrintConfig &printer_config = wxGetApp().preset_bundle->printers.get_selected_preset().config;
        auto *nozzle_opt = printer_config.option<ConfigOptionFloats>("nozzle_diameter");
        double nozzle_diam = (nozzle_opt && !nozzle_opt->values.empty()) ? nozzle_opt->values[0] : 0.4;

        double ref_width = 0.0;
        auto *width_opt = config->option<ConfigOptionFloatOrPercent>(ref_width_key);
        if (width_opt)
        {
            if (width_opt->percent)
            {
                // Width is percentage of nozzle - resolve it
                ref_width = nozzle_diam * width_opt->value / 100.0;
            }
            else if (width_opt->value > 0)
            {
                ref_width = width_opt->value;
            }
        }
        // If width is 0 (auto) or very small, use nozzle diameter as reference
        if (ref_width < 0.1)
        {
            ref_width = nozzle_diam;
        }

        double min_mm = ref_width * min_percent / 100.0;
        double max_mm = ref_width * max_percent / 100.0;

        bool needs_clamp = false;
        bool exceeded_max = false;
        double new_value = overlap_opt->value;
        bool new_percent = overlap_opt->percent;

        if (overlap_opt->percent)
        {
            // Percentage mode
            if (overlap_opt->value > max_percent)
            {
                new_value = max_percent;
                needs_clamp = true;
                exceeded_max = true;
            }
            else if (overlap_opt->value < min_percent)
            {
                new_value = min_percent;
                needs_clamp = true;
                exceeded_max = false;
            }
        }
        else
        {
            // Absolute mm mode
            if (overlap_opt->value > max_mm + 0.001)
            {
                new_value = max_mm;
                needs_clamp = true;
                exceeded_max = true;
            }
            else if (overlap_opt->value < min_mm - 0.001)
            {
                new_value = min_mm;
                needs_clamp = true;
                exceeded_max = false;
            }
        }

        if (needs_clamp)
        {
            // Build descriptive message
            wxString limit_desc;
            if (exceeded_max)
            {
                if (max_percent == 100.0)
                    limit_desc = wxString::Format(_L("cannot be greater than %s"), ref_width_label);
                else
                    limit_desc = wxString::Format(_L("cannot be greater than %d%% of %s"), (int) max_percent,
                                                  ref_width_label);
            }
            else
            {
                if (min_percent == -100.0)
                    limit_desc = wxString::Format(_L("cannot be less than -%s (negative %s)"), ref_width_label,
                                                  ref_width_label);
                else
                    limit_desc = wxString::Format(_L("cannot be less than %d%% of %s"), (int) min_percent,
                                                  ref_width_label);
            }

            wxString new_value_str;
            if (new_percent)
                new_value_str = wxString::Format("%.2f%%", new_value);
            else
                new_value_str = wxString::Format("%.3f mm", new_value);

            wxString msg_text = wxString::Format(_L("%s %s.\n\nThe value has been set to %s."), label, limit_desc,
                                                 new_value_str);

            WarningDialog dialog(m_msg_dlg_parent, msg_text, _L("Parameter validation") + ": " + opt_key, wxOK);
            DynamicPrintConfig new_conf = *config;
            new_conf.set_key_value(opt_key, new ConfigOptionFloatOrPercent(new_value, new_percent));
            is_msg_dlg_already_exist = true;
            dialog.ShowModal();
            apply(config, &new_conf);
            is_msg_dlg_already_exist = false;
        }
    };

    // Clamp external perimeter overlap: -100% to 100%
    clamp_overlap("external_perimeter_overlap", "perimeter_extrusion_width", -100.0, 100.0,
                  _L("External perimeter/perimeter overlap").ToStdString(),
                  _L("Perimeter extrusion width").ToStdString());

    // Clamp perimeter/perimeter overlap: -100% to 80%
    clamp_overlap("perimeter_perimeter_overlap", "perimeter_extrusion_width", -100.0, 80.0,
                  _L("Perimeter/perimeter overlap").ToStdString(), _L("Perimeter extrusion width").ToStdString());

    // Clamp infill/perimeters overlap: -100% to 100%
    clamp_overlap("infill_overlap", "perimeter_extrusion_width", -100.0, 100.0,
                  _L("Infill/perimeters overlap").ToStdString(), _L("Perimeter extrusion width").ToStdString());

    // Clamp bridge infill/perimeters overlap: -100% to 100%
    clamp_overlap("bridge_infill_perimeter_overlap", "perimeter_extrusion_width", -100.0, 100.0,
                  _L("Bridge infill/perimeters overlap").ToStdString(), _L("Perimeter extrusion width").ToStdString());

    // Clamp bridge infill overlap: -100% to 80%
    clamp_overlap("bridge_infill_overlap", "bridge_extrusion_width", -100.0, 80.0,
                  _L("Bridge infill overlap").ToStdString(), _L("Bridge extrusion width").ToStdString());

    // Note: s_approved_narrow_widths and s_approved_wide_widths are now file-scope statics (s_approved_*)
    // to allow pre-approval via ConfigManipulation::approve_extrusion_width()

    // BOSS: State for "Yes to All" / "No to All" across all extrusion width validations
    bool approve_all_widths = false;
    bool reset_all_widths = false;

    // Helper lambda to validate extrusion width against its corresponding nozzle
    auto validate_extrusion_width =
        [&](const std::string &width_key, const std::string &extruder_key, const std::string &label)
    {
        if (is_msg_dlg_already_exist)
            return;

        // Skip validation during initial app load (user hasn't configured anything yet)
        if (s_suppress_extrusion_width_warnings)
            return;

        // Skip validation if the changed key is not related to extrusion widths
        // This prevents warnings when user changes unrelated settings like "perimeters"
        if (!should_validate_extrusion_widths)
            return;

        auto *width_opt = config->option<ConfigOptionFloatOrPercent>(width_key);
        if (!width_opt)
            return;

        // Get the extruder index (1-based in config, convert to 0-based for nozzle array)
        int extruder_idx = 0;
        if (!extruder_key.empty())
        {
            auto *extruder_opt = config->option<ConfigOptionInt>(extruder_key);
            if (extruder_opt && extruder_opt->value > 0)
                extruder_idx = extruder_opt->value - 1;
        }

        // Get nozzle diameter from printer config (not print config)
        // Must use get_edited_preset() not get_selected_preset() because changes are in
        // the edited preset until saved. Selected preset has the old/saved values.
        const DynamicPrintConfig &printer_config = wxGetApp().preset_bundle->printers.get_edited_preset().config;
        auto *nozzle_opt = printer_config.option<ConfigOptionFloats>("nozzle_diameter");
        if (!nozzle_opt || nozzle_opt->values.empty())
            return;

        double nozzle_diam = nozzle_opt->values[std::min(extruder_idx, (int) nozzle_opt->values.size() - 1)];
        if (nozzle_diam < 0.1)
            return; // Invalid nozzle

        // Calculate the actual width in mm
        double width_mm = 0.0;
        if (width_opt->percent)
        {
            width_mm = nozzle_diam * width_opt->value / 100.0;
        }
        else
        {
            width_mm = width_opt->value;
        }

        // If width is effectively 0 (< 0.001), normalize to 0 (auto)
        if (width_mm < 0.001)
        {
            if (width_opt->value != 0.0)
            {
                // Update UI to show 0
                DynamicPrintConfig new_conf = *config;
                new_conf.set_key_value(width_key, new ConfigOptionFloatOrPercent(0.0, false));
                apply(config, &new_conf);
            }
            return;
        }

        double min_width = nozzle_diam * 0.6; // 60% of nozzle
        double max_width = nozzle_diam * 1.5; // 150% of nozzle

        // Check if width is within valid range (60% - 150%)
        bool is_too_narrow = width_mm < min_width - 0.001;
        bool is_too_wide = width_mm > max_width + 0.001;

        if (!is_too_narrow && !is_too_wide)
        {
            // Width is valid - remove from approved lists if it was there
            s_approved_narrow_widths.erase(width_key);
            s_approved_wide_widths.erase(width_key);
            return;
        }

        // Check if this exact value was already approved
        if (is_too_narrow)
        {
            auto it = s_approved_narrow_widths.find(width_key);
            if (it != s_approved_narrow_widths.end() && std::abs(it->second - width_mm) < 0.0001)
            {
                return; // Already approved
            }
        }
        if (is_too_wide)
        {
            auto it = s_approved_wide_widths.find(width_key);
            if (it != s_approved_wide_widths.end() && std::abs(it->second - width_mm) < 0.0001)
            {
                return; // Already approved
            }
        }

        // Build warning message
        wxString width_str;
        if (width_opt->percent)
            width_str = wxString::Format("%.0f%%", width_opt->value);
        else
            width_str = wxString::Format("%.3f mm", width_opt->value);

        wxString msg_text;
        if (is_too_narrow)
        {
            msg_text = wxString::Format(_L("%s is set to %s, which is below 60%% of the nozzle diameter (%.2f mm).\n\n"
                                           "Extrusion widths below 60%% of nozzle size may cause printing issues.\n\n"
                                           "Do you want to keep this value?\n"
                                           "Select YES to keep %s,\n"
                                           "or NO to reset to %.2f mm (nozzle diameter)."),
                                        label, width_str, nozzle_diam, width_str, nozzle_diam);
        }
        else
        {
            msg_text = wxString::Format(_L("%s is set to %s, which exceeds 150%% of the nozzle diameter (%.2f mm).\n\n"
                                           "Extrusion widths above 150%% of nozzle size may cause printing issues.\n\n"
                                           "Do you want to keep this value?\n"
                                           "Select YES to keep %s,\n"
                                           "or NO to reset to %.2f mm (nozzle diameter)."),
                                        label, width_str, nozzle_diam, width_str, nozzle_diam);
        }

        // BOSS: Handle "to All" state from a previous iteration
        bool keep = false;
        if (approve_all_widths)
            keep = true;
        else if (reset_all_widths)
            keep = false;
        else
        {
            // Show 4-button dialog: Yes / Yes to All / No / No to All
            enum
            {
                ID_YES_TO_ALL = wxID_HIGHEST + 1,
                ID_NO_TO_ALL
            };

            wxDialog dialog(m_msg_dlg_parent, wxID_ANY, _L("Parameter validation") + ": " + width_key,
                            wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
            wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
            wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);

            sizer->Add(new wxStaticText(&dialog, wxID_ANY, msg_text), 0, wxALL, 15);

            btn_sizer->AddStretchSpacer();
            auto add_btn = [&](wxWindowID id, const wxString &lbl, bool focus)
            {
                wxButton *btn = new wxButton(&dialog, id, lbl);
                if (focus)
                {
                    btn->SetFocus();
                    btn->SetDefault();
                }
                btn_sizer->Add(btn, 0, wxLEFT, 5);
                btn->Bind(wxEVT_BUTTON, [&dialog, id](wxCommandEvent &) { dialog.EndModal(id); });
            };
            add_btn(wxID_YES, _L("Yes"), true);
            add_btn(ID_YES_TO_ALL, _L("Yes to All"), false);
            add_btn(wxID_NO, _L("No"), false);
            add_btn(ID_NO_TO_ALL, _L("No to All"), false);

            sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
            dialog.SetSizerAndFit(sizer);
            wxGetApp().UpdateDlgDarkUI(&dialog);
            dialog.CenterOnParent();

            is_msg_dlg_already_exist = true;
            int result = dialog.ShowModal();
            is_msg_dlg_already_exist = false;

            if (result == ID_YES_TO_ALL)
            {
                approve_all_widths = true;
                keep = true;
            }
            else if (result == ID_NO_TO_ALL)
            {
                reset_all_widths = true;
                keep = false;
            }
            else
                keep = (result == wxID_YES);
        }

        if (keep)
        {
            // User approved this out-of-range width - remember it
            if (is_too_narrow)
                s_approved_narrow_widths[width_key] = width_mm;
            else
                s_approved_wide_widths[width_key] = width_mm;
        }
        else
        {
            // User rejected - reset to nozzle diameter
            s_approved_narrow_widths.erase(width_key);
            s_approved_wide_widths.erase(width_key);
            DynamicPrintConfig new_conf = *config;
            new_conf.set_key_value(width_key, new ConfigOptionFloatOrPercent(nozzle_diam, false));
            apply(config, &new_conf);
        }
    };

    // Validate all extrusion widths against their corresponding nozzles
    validate_extrusion_width("extrusion_width", "", _L("Default extrusion width").ToStdString());
    validate_extrusion_width("first_layer_extrusion_width", "", _L("First layer extrusion width").ToStdString());
    validate_extrusion_width("perimeter_extrusion_width", "perimeter_extruder",
                             _L("Perimeter extrusion width").ToStdString());
    validate_extrusion_width("external_perimeter_extrusion_width", "perimeter_extruder",
                             _L("External perimeter extrusion width").ToStdString());
    validate_extrusion_width("infill_extrusion_width", "infill_extruder", _L("Infill extrusion width").ToStdString());
    validate_extrusion_width("solid_infill_extrusion_width", "solid_infill_extruder",
                             _L("Solid infill extrusion width").ToStdString());
    validate_extrusion_width("top_infill_extrusion_width", "solid_infill_extruder",
                             _L("Top infill extrusion width").ToStdString());
    validate_extrusion_width("support_material_extrusion_width", "support_material_extruder",
                             _L("Support material extrusion width").ToStdString());
    validate_extrusion_width("support_material_interface_extrusion_width", "support_material_interface_extruder",
                             _L("Support material interface extrusion width").ToStdString());
    validate_extrusion_width("bridge_extrusion_width", "perimeter_extruder",
                             _L("Bridge extrusion width").ToStdString());


    double fill_density = config->option<ConfigOptionPercent>("fill_density")->value;

    if (config->opt_bool("spiral_vase") &&
        ! (config->opt_int("perimeters") == 1 &&
           config->opt_int("top_solid_layers") == 0 &&
           fill_density == 0 &&
           ! config->opt_bool("support_material") &&
           config->opt_int("support_material_enforce_layers") == 0 &&
           ! config->opt_bool("thin_walls") &&
           ! config->opt_bool("overhangs_reverse") &&
           ! config->opt_bool("infill_reverse") &&
           ! config->opt_bool("internal_perimeters_reverse")))
    {
        wxString msg_text = _(L("The Spiral Vase mode requires:\n"
                                "- one perimeter\n"
                                "- no extra perimeter on odd layers\n"
                                "- no reverse on odd layers\n"
                                "- no top solid layers\n"
                                "- 0% fill density\n"
                                "- no support material\n"
               					"- Detect thin walls disabled"));
        if (is_global_config)
            msg_text += "\n\n" + _(L("Shall I adjust those settings in order to enable Spiral Vase?"));
        MessageDialog dialog(m_msg_dlg_parent, msg_text, _(L("Spiral Vase")),
                               wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK));
        DynamicPrintConfig new_conf = *config;
        auto answer = dialog.ShowModal();
        bool support = true;
        if (!is_global_config || answer == wxID_YES) {
            new_conf.set_key_value("perimeters", new ConfigOptionInt(1));
            new_conf.set_key_value("top_solid_layers", new ConfigOptionInt(0));
            new_conf.set_key_value("fill_density", new ConfigOptionPercent(0));
            new_conf.set_key_value("support_material", new ConfigOptionBool(false));
            new_conf.set_key_value("support_material_enforce_layers", new ConfigOptionInt(0));
            new_conf.set_key_value("thin_walls", new ConfigOptionBool(false));
            new_conf.set_key_value("overhangs_reverse", new ConfigOptionBool(false));
            new_conf.set_key_value("infill_reverse", new ConfigOptionBool(false));
            new_conf.set_key_value("internal_perimeters_reverse", new ConfigOptionBool(false));
            fill_density = 0;
            support = false;
        }
        else {
            new_conf.set_key_value("spiral_vase", new ConfigOptionBool(false));
        }
        apply(config, &new_conf);
        if (cb_value_change) {
            cb_value_change("fill_density", fill_density);
            if (!support)
                cb_value_change("support_material", false);
        }
    }

    if (config->opt_bool("alternate_extra_perimeter") &&
        config->opt_enum<EnsureVerticalShellThickness>("ensure_vertical_shell_thickness") ==
            EnsureVerticalShellThickness::Enabled) {
        wxString msg_text = _(L("Alternate extra perimeters doesn't work well with ensure vertical "
                                "shell thickness enabled. "));

        if (is_global_config)
            msg_text += "\n\n" + _(L("Change these settings automatically? \n"
                                     "Yes - Change ensure vertical shell thickness to Partial and enable alternate extra perimeters\n"
                                     "No  - Don't use alternate extra perimeters"));

        MessageDialog dialog(m_msg_dlg_parent, msg_text, "",
                               wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK));
        DynamicPrintConfig new_conf = *config;
        auto answer = dialog.ShowModal();
        if (!is_global_config || answer == wxID_YES) {
            new_conf.set_key_value("ensure_vertical_shell_thickness", new ConfigOptionEnum<EnsureVerticalShellThickness>(EnsureVerticalShellThickness::Partial));
            new_conf.set_key_value("alternate_extra_perimeter", new ConfigOptionBool(true));
        }
        else {
            new_conf.set_key_value("ensure_vertical_shell_thickness", new ConfigOptionEnum<EnsureVerticalShellThickness>(EnsureVerticalShellThickness::Enabled));
            new_conf.set_key_value("alternate_extra_perimeter", new ConfigOptionBool(false));
        }
        apply(config, &new_conf);
    }

    if (config->opt_bool("wipe_tower") && config->opt_bool("support_material") && 
        // Organic supports are always synchronized with object layers as of now.
        config->opt_enum<SupportMaterialStyle>("support_material_style") != smsOrganic) {
        if (config->opt_float("support_material_contact_distance") == 0) {
            if (!config->opt_bool("support_material_synchronize_layers")) {
                wxString msg_text = _(L("For the Wipe Tower to work with the soluble supports, the support layers\n"
                                        "need to be synchronized with the object layers."));
                if (is_global_config)
                    msg_text += "\n\n" + _(L("Shall I synchronize support layers in order to enable the Wipe Tower?"));
                MessageDialog dialog(m_msg_dlg_parent, msg_text, _(L("Wipe Tower")),
                                       wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK));
                DynamicPrintConfig new_conf = *config;
                auto answer = dialog.ShowModal();
                if (!is_global_config || answer == wxID_YES) {
                    new_conf.set_key_value("support_material_synchronize_layers", new ConfigOptionBool(true));
                }
                else
                    new_conf.set_key_value("wipe_tower", new ConfigOptionBool(false));
                apply(config, &new_conf);
            }
        } else {
            if ((config->opt_int("support_material_extruder") != 0 || config->opt_int("support_material_interface_extruder") != 0)) {
                wxString msg_text = _(L("The Wipe Tower currently supports the non-soluble supports only "
                                        "if they are printed with the current extruder without triggering a tool change. "
                                        "(both support_material_extruder and support_material_interface_extruder need to be set to 0)."));
                if (is_global_config)
                    msg_text += "\n\n" + _(L("Shall I adjust those settings in order to enable the Wipe Tower?"));
                MessageDialog dialog (m_msg_dlg_parent, msg_text, _(L("Wipe Tower")),
                                        wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK));
                DynamicPrintConfig new_conf = *config;
                auto answer = dialog.ShowModal();
                if (!is_global_config || answer == wxID_YES) {
                    new_conf.set_key_value("support_material_extruder", new ConfigOptionInt(0));
                    new_conf.set_key_value("support_material_interface_extruder", new ConfigOptionInt(0));
                }
                else
                    new_conf.set_key_value("wipe_tower", new ConfigOptionBool(false));
                apply(config, &new_conf);
            }
        }
    }

    // Check "support_material" and "overhangs" relations only on global settings level
    if (is_global_config && config->opt_bool("support_material")) {
        // Ask only once.
        if (!m_support_material_overhangs_queried) {
            m_support_material_overhangs_queried = true;
            if (!config->opt_bool("overhangs")/* != 1*/) {
                wxString msg_text = _(L("Supports work better, if the following feature is enabled:\n"
                                        "- Detect bridging perimeters"));
                if (is_global_config)
                    msg_text += "\n\n" + _(L("Shall I adjust those settings for supports?"));
                MessageDialog dialog(m_msg_dlg_parent, msg_text, _L("Support Generator"), wxICON_WARNING | wxYES | wxNO);
                DynamicPrintConfig new_conf = *config;
                auto answer = dialog.ShowModal();
                if (answer == wxID_YES) {
                    // Enable "detect bridging perimeters".
                    new_conf.set_key_value("overhangs", new ConfigOptionBool(true));
                }
                //else Do nothing, leave supports on and "detect bridging perimeters" off.
                apply(config, &new_conf);
            }
        }
    }
    else {
        m_support_material_overhangs_queried = false;
    }

    if (config->option<ConfigOptionPercent>("fill_density")->value == 100) {
        const int fill_pattern = config->option<ConfigOptionEnum<InfillPattern>>("fill_pattern")->value;
        bool correct_100p_fill = config->option_def("top_fill_pattern")->enum_def->enum_to_index(fill_pattern).has_value();
        if (!correct_100p_fill) {
            correct_100p_fill = config->option_def("bottom_fill_pattern")->enum_def->enum_to_index(fill_pattern).has_value();
        }
        if (!correct_100p_fill) {
            correct_100p_fill = config->option_def("solid_fill_pattern")->enum_def->enum_to_index(fill_pattern).has_value();
        }
        if (!correct_100p_fill) {
            // get fill_pattern name from enum_labels for using this one at dialog_msg
            const ConfigOptionDef *fill_pattern_def = config->option_def("fill_pattern");
            assert(fill_pattern_def != nullptr);
            if (auto label = fill_pattern_def->enum_def->enum_to_label(fill_pattern); label.has_value()) {
                wxString msg_text = GUI::format_wxstr(_L("The %1% infill pattern is not supposed to work at 100%% density."), _(*label));
                if (is_global_config)
                    msg_text += "\n\n" + _L("Shall I switch to rectilinear fill pattern?");
                MessageDialog dialog(m_msg_dlg_parent, msg_text, _L("Infill"),
                                                  wxICON_WARNING | (is_global_config ? wxYES | wxNO : wxOK) );
                DynamicPrintConfig new_conf = *config;
                auto answer = dialog.ShowModal();
                if (!is_global_config || answer == wxID_YES) {
                    new_conf.set_key_value("fill_pattern", new ConfigOptionEnum<InfillPattern>(ipRectilinear));
                    fill_density = 100;
                }
                else
                    fill_density = wxGetApp().preset_bundle->prints.get_selected_preset().config.option<ConfigOptionPercent>("fill_density")->value;
                new_conf.set_key_value("fill_density", new ConfigOptionPercent(fill_density));
                apply(config, &new_conf);
                if (cb_value_change)
                    cb_value_change("fill_density", fill_density);
            }
        }
    }

    if (config->opt_bool("automatic_extrusion_widths")) {
        std::optional<DynamicPrintConfig> new_config = handle_automatic_extrusion_widths(*config, is_global_config, m_msg_dlg_parent);
        if (new_config.has_value()) {
            apply(config, &(*new_config));
        }
    }
}

void ConfigManipulation::toggle_print_fff_options(DynamicPrintConfig* config)
{
    PresetBundle *preset_bundle  = wxGetApp().preset_bundle;

    auto gcflavor = preset_bundle->printers.get_selected_preset().config.option<ConfigOptionEnum<GCodeFlavor>>("gcode_flavor")->value;

    bool have_perimeters = config->opt_int("perimeters") > 0;
    for (auto el : { "extra_perimeters","extra_perimeters_on_overhangs", "thin_walls", "overhangs",
                    "seam_position","staggered_inner_seams", "external_perimeters_first", "external_perimeter_extrusion_width",
                    "perimeter_speed", "small_perimeter_speed", "external_perimeter_speed", "enable_dynamic_overhang_speeds",
                    "small_perimeter_min_length", "small_perimeter_max_length", "external_perimeters_first_holes",
                    "external_perimeters_first_holes_min_size", "external_perimeters_first_disabled_first_layers",
                    // BOSS
                    "seam_type", "seam_notch_width", "seam_notch_angle",
                })
        toggle_field(el, have_perimeters);

    bool has_seam_notch = have_perimeters && config->opt_enum<SeamNotchType>("seam_type") != sntRegular;
    toggle_field("seam_notch_width", has_seam_notch);
    toggle_field("seam_notch_angle", has_seam_notch);

    for (size_t i = 0; i < 4; i++) {
        toggle_field("overhang_speed_" + std::to_string(i), config->opt_bool("enable_dynamic_overhang_speeds"));
    }

    const bool have_infill                      = config->option<ConfigOptionPercent>("fill_density")->value > 0;
    const bool has_automatic_infill_combination = config->option<ConfigOptionBool>("automatic_infill_combination")->value;
    // infill_extruder uses the same logic as in Print::extruders()
    for (auto el : { "fill_pattern","solid_infill_every_layers", "solid_infill_below_area", "infill_extruder",
                    "infill_anchor_max", "automatic_infill_combination" }) {
        toggle_field(el, have_infill);
    }

    toggle_field("infill_every_layers", have_infill && !has_automatic_infill_combination);
    toggle_field("automatic_infill_combination_max_layer_height", have_infill && has_automatic_infill_combination);

    // Only allow configuration of open anchors if the anchoring is enabled.
    bool has_infill_anchors = have_infill && config->option<ConfigOptionFloatOrPercent>("infill_anchor_max")->value > 0;
    toggle_field("infill_anchor", has_infill_anchors);

    bool has_spiral_vase         = config->opt_bool("spiral_vase");
    bool has_top_solid_infill 	 = config->opt_int("top_solid_layers") > 0;
    bool has_bottom_solid_infill = config->opt_int("bottom_solid_layers") > 0;
    bool has_solid_infill 		 = has_top_solid_infill || has_bottom_solid_infill;
    // solid_infill_extruder uses the same logic as in Print::extruders()
    for (auto el : { "top_fill_pattern", "bottom_fill_pattern", "solid_fill_pattern", "infill_first", "solid_infill_extruder",
                    "solid_infill_extrusion_width", "solid_infill_speed" })
        toggle_field(el, has_solid_infill);

    bool has_narrow_detection = has_solid_infill && config->opt_bool("detect_narrow_solid_infill");
    toggle_field("detect_narrow_solid_infill", has_solid_infill);
    toggle_field("detect_narrow_solid_infill_threshold", has_narrow_detection);

    for (auto el : { "fill_angle", "bridge_angle", "infill_extrusion_width",
                    "infill_speed", "bridge_speed", "over_bridge_speed" })
        toggle_field(el, have_infill || has_solid_infill);

    for (auto el : { "small_perimeter_min_length", "small_perimeter_max_length" })
        toggle_field(el, config->option<ConfigOptionFloatOrPercent>("small_perimeter_speed")->value > 0);

    const bool has_ensure_vertical_shell_thickness = config->opt_enum<EnsureVerticalShellThickness>("ensure_vertical_shell_thickness") != EnsureVerticalShellThickness::Disabled;
    toggle_field("top_solid_min_thickness", !has_spiral_vase && has_top_solid_infill && has_ensure_vertical_shell_thickness);
    toggle_field("bottom_solid_min_thickness", !has_spiral_vase && has_bottom_solid_infill && has_ensure_vertical_shell_thickness);

    // Gap fill is newly allowed in between perimeter lines even for empty infill (see GH #1476).
    toggle_field("gap_fill_speed", have_perimeters);

    for (auto el : { "top_infill_extrusion_width", "top_solid_infill_speed" })
        toggle_field(el, has_top_solid_infill || (has_spiral_vase && has_bottom_solid_infill));

    bool have_default_acceleration = config->opt_float("default_acceleration") > 0;
    for (auto el : { "perimeter_acceleration", "infill_acceleration", "top_solid_infill_acceleration",
                    "solid_infill_acceleration", "external_perimeter_acceleration",
                    "bridge_acceleration", "first_layer_acceleration", "wipe_tower_acceleration"})
        toggle_field(el, have_default_acceleration);

    bool have_default_jerk = config->opt_int("default_jerk") > 0;
    for (auto el : { "perimeter_jerk", "infill_jerk", "top_solid_infill_jerk",
                    "solid_infill_jerk", "external_perimeter_jerk", "bridge_jerk",
                    "first_layer_jerk", "first_layer_jerk_over_raft", "wipe_tower_jerk", "travel_jerk" })
        toggle_field(el, have_default_jerk);

    bool have_minimum_cruise_ratio = config->opt_float("default_minimum_cruise_ratio") > 0 && gcflavor == gcfKlipper;
    for (auto el : { "perimeter_minimum_cruise_ratio", "infill_minimum_cruise_ratio", "top_solid_infill_minimum_cruise_ratio",
                    "solid_infill_minimum_cruise_ratio", "external_perimeter_minimum_cruise_ratio", "bridge_minimum_cruise_ratio",
                    "first_layer_minimum_cruise_ratio", "first_layer_minimum_cruise_ratio_over_raft", "travel_minimum_cruise_ratio",
                    "wipe_tower_minimum_cruise_ratio" })
        toggle_field(el, have_minimum_cruise_ratio);

    bool have_skirt = config->opt_int("skirts") > 0;
    toggle_field("skirt_height", have_skirt && config->opt_enum<DraftShield>("draft_shield") != dsEnabled);
    for (auto el : { "skirt_distance", "draft_shield", "min_skirt_length" })
        toggle_field(el, have_skirt);

    bool have_brim = config->opt_enum<BrimType>("brim_type") != btNoBrim;
    for (auto el : { "brim_width", "brim_separation" })
        toggle_field(el, have_brim);
    // perimeter_extruder uses the same logic as in Print::extruders()
    toggle_field("perimeter_extruder", have_perimeters || have_brim);

    bool have_raft = config->opt_int("raft_layers") > 0;
    bool have_support_material = config->opt_bool("support_material") || have_raft;
    bool have_support_material_auto = have_support_material && config->opt_bool("support_material_auto");
    bool have_support_interface = config->opt_int("support_material_interface_layers") > 0;
    bool have_support_soluble = have_support_material && config->opt_float("support_material_contact_distance") == 0;
    auto support_material_style = config->opt_enum<SupportMaterialStyle>("support_material_style");
    for (auto el : { "support_material_style", "support_material_pattern", "support_material_with_sheath",
                    "support_material_spacing", "support_material_angle", 
                    "support_material_interface_pattern", "support_material_interface_layers",
                    "dont_support_bridges", "support_material_extrusion_width", "support_material_contact_distance",
                    "support_material_xy_spacing" })
        toggle_field(el, have_support_material);
    toggle_field("support_material_threshold", have_support_material_auto);
    toggle_field("support_material_bottom_contact_distance", have_support_material && ! have_support_soluble);
    toggle_field("support_material_closing_radius", have_support_material && support_material_style == smsSnug);

    const bool has_organic_supports = support_material_style == smsOrganic && 
                                     (config->opt_bool("support_material") || 
                                      config->opt_int("support_material_enforce_layers") > 0);
    for (const std::string& key : { "support_tree_angle", "support_tree_angle_slow", "support_tree_branch_diameter",
                                    "support_tree_branch_diameter_angle", "support_tree_branch_diameter_double_wall", 
                                    "support_tree_tip_diameter", "support_tree_branch_distance", "support_tree_top_rate" })
        toggle_field(key, has_organic_supports);

    for (auto el : { "support_material_bottom_interface_layers", "support_material_interface_spacing", "support_material_interface_extruder",
                    "support_material_interface_speed", "support_material_interface_contact_loops" })
        toggle_field(el, have_support_material && have_support_interface);
    toggle_field("support_material_synchronize_layers", have_support_soluble);

    toggle_field("perimeter_extrusion_width", have_perimeters || have_skirt || have_brim);
    toggle_field("support_material_extruder", have_support_material || have_skirt);
    toggle_field("support_material_speed", have_support_material || have_brim || have_skirt);

    toggle_field("raft_contact_distance", have_raft && !have_support_soluble);
    for (auto el : { "raft_expansion", "first_layer_speed_over_raft" })
        toggle_field(el, have_raft);

    // for default_extrusion_width/spacing, you need to have at least an extrusion_width with 0
    bool have_default_width = config->option("first_layer_extrusion_width")->getFloat() == 0 ||
        (config->option("perimeter_extrusion_width")->getFloat() == 0 && (have_perimeters || have_brim)) ||
        (config->option("external_perimeter_extrusion_width")->getFloat() == 0 && have_perimeters) ||
        (config->option("infill_extrusion_width")->getFloat() == 0 && (have_infill || has_solid_infill)) ||
        (config->option("solid_infill_extrusion_width")->getFloat() == 0 && has_solid_infill) ||
        (config->option("top_infill_extrusion_width")->getFloat() == 0 && has_top_solid_infill) ||
        (config->option("support_material_extrusion_width")->getFloat() == 0 && have_support_material);
    toggle_field("extrusion_width", have_default_width);

    bool has_ironing = config->opt_bool("ironing");
    for (auto el : { "ironing_type", "ironing_flowrate", "ironing_spacing", "ironing_speed" })
    	toggle_field(el, has_ironing);

    bool has_fuzzy_skin = config->opt_enum<FuzzySkinType>("fuzzy_skin") != FuzzySkinType::None;
    for (auto el : { "fuzzy_skin_thickness", "fuzzy_skin_point_dist", "fuzzy_skin_noise_type" })
        toggle_field(el, has_fuzzy_skin);
    auto fuzzy_noise_type = config->opt_enum<FuzzySkinNoiseType>("fuzzy_skin_noise_type");
    bool has_structured_noise = has_fuzzy_skin && fuzzy_noise_type != FuzzySkinNoiseType::Classic;
    toggle_field("fuzzy_skin_feature_size", has_structured_noise);
    toggle_field("fuzzy_skin_octaves", has_structured_noise && fuzzy_noise_type != FuzzySkinNoiseType::Voronoi);
    toggle_field("fuzzy_skin_persistence", has_structured_noise &&
        (fuzzy_noise_type == FuzzySkinNoiseType::Perlin || fuzzy_noise_type == FuzzySkinNoiseType::Billow));

    bool have_ooze_prevention = config->opt_bool("ooze_prevention");
    toggle_field("standby_temperature_delta", have_ooze_prevention);
    toggle_field("preheat_time", have_ooze_prevention);
    toggle_field("preheat_steps", have_ooze_prevention);

    bool have_wipe_tower = config->opt_bool("wipe_tower");
    for (auto el : { "wipe_tower_width",  "wipe_tower_brim_width", "wipe_tower_cone_angle",
                     "wipe_tower_extra_spacing", "wipe_tower_extra_flow", "wipe_tower_bridging", "wipe_tower_no_sparse_layers", "single_extruder_multi_material_priming",
                     // BOSS
                     "wipe_tower_disable_filament_ramming", "wipe_tower_disable_cooling_moves", "wipe_tower_max_purge_speed", "wipe_tower_disable_linear_advance" })
        toggle_field(el, have_wipe_tower);

    toggle_field("avoid_crossing_curled_overhangs", !config->opt_bool("avoid_crossing_perimeters"));
    toggle_field("avoid_crossing_perimeters", !config->opt_bool("avoid_crossing_curled_overhangs"));

    bool have_avoid_crossing_perimeters = config->opt_bool("avoid_crossing_perimeters");
    toggle_field("avoid_crossing_perimeters_max_detour", have_avoid_crossing_perimeters);

    bool have_arachne = config->opt_enum<PerimeterGeneratorType>("perimeter_generator") == PerimeterGeneratorType::Arachne;
    toggle_field("wall_transition_length", have_arachne);
    toggle_field("wall_transition_filter_deviation", have_arachne);
    toggle_field("wall_transition_angle", have_arachne);
    toggle_field("wall_distribution_count", have_arachne);
    toggle_field("min_feature_size", have_arachne);
    toggle_field("min_bead_width", have_arachne);
    toggle_field("thin_walls", !have_arachne);

    toggle_field("scarf_seam_placement", !has_spiral_vase);
    const auto scarf_seam_placement{config->opt_enum<ScarfSeamPlacement>("scarf_seam_placement")};
    const bool uses_scarf_seam{!has_spiral_vase && scarf_seam_placement != ScarfSeamPlacement::nowhere};
    toggle_field("scarf_seam_only_on_smooth", uses_scarf_seam);
    toggle_field("scarf_seam_start_height", uses_scarf_seam);
    toggle_field("scarf_seam_entire_loop", uses_scarf_seam);
    toggle_field("scarf_seam_length", uses_scarf_seam);
    toggle_field("scarf_seam_max_segment_length", uses_scarf_seam);
    toggle_field("scarf_seam_on_inner_perimeters", uses_scarf_seam);

    bool use_beam_interlocking = config->opt_bool("interlocking_beam");
    toggle_field("interlocking_beam_width", use_beam_interlocking);
    toggle_field("interlocking_orientation", use_beam_interlocking);
    toggle_field("interlocking_beam_layer_count", use_beam_interlocking);
    toggle_field("interlocking_depth", use_beam_interlocking);
    toggle_field("interlocking_boundary_avoidance", use_beam_interlocking);
    toggle_field("mmu_segmented_region_max_width", !use_beam_interlocking);

    bool have_non_zero_mmu_segmented_region_max_width = !use_beam_interlocking && config->opt_float("mmu_segmented_region_max_width") > 0.;
    toggle_field("mmu_segmented_region_interlocking_depth", have_non_zero_mmu_segmented_region_max_width);

    bool have_small_area_infill_flow_compensation = config->opt_bool("small_area_infill_flow_compensation");
    toggle_field("small_area_infill_flow_compensation_extrusion_length_0", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_1", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_2", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_3", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_4", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_5", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_6", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_7", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_8", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_extrusion_length_9", have_small_area_infill_flow_compensation);

    toggle_field("small_area_infill_flow_compensation_compensation_factor_0", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_1", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_2", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_3", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_4", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_5", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_6", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_7", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_8", have_small_area_infill_flow_compensation);
    toggle_field("small_area_infill_flow_compensation_compensation_factor_9", have_small_area_infill_flow_compensation);

    toggle_field("external_perimeters_first_holes", config->opt_bool("external_perimeters_first"));
    toggle_field("external_perimeters_first_disabled_first_layers", config->opt_bool("external_perimeters_first"));
    toggle_field("external_perimeters_first_holes_min_size", config->opt_bool("external_perimeters_first") && config->opt_bool("external_perimeters_first_holes"));
}

void ConfigManipulation::toggle_print_sla_options(DynamicPrintConfig* config)
{
    bool supports_en = config->opt_bool("supports_enable");
    sla::SupportTreeType treetype = config->opt_enum<sla::SupportTreeType>("support_tree_type");
    bool is_default_tree = treetype == sla::SupportTreeType::Default;
    bool is_branching_tree = treetype == sla::SupportTreeType::Branching;

    toggle_field("support_tree_type", supports_en);

    toggle_field("support_head_front_diameter", supports_en && is_default_tree);
    toggle_field("support_head_penetration", supports_en && is_default_tree);
    toggle_field("support_head_width", supports_en && is_default_tree);
    toggle_field("support_pillar_diameter", supports_en && is_default_tree);
    toggle_field("support_small_pillar_diameter_percent", supports_en && is_default_tree);
    toggle_field("support_max_bridges_on_pillar", supports_en && is_default_tree);
    toggle_field("support_pillar_connection_mode", supports_en && is_default_tree);
    toggle_field("support_buildplate_only", supports_en && is_default_tree);
    toggle_field("support_base_diameter", supports_en && is_default_tree);
    toggle_field("support_base_height", supports_en && is_default_tree);
    toggle_field("support_base_safety_distance", supports_en && is_default_tree);
    toggle_field("support_critical_angle", supports_en && is_default_tree);
    toggle_field("support_max_bridge_length", supports_en && is_default_tree);
    toggle_field("support_enforcers_only", supports_en);
    toggle_field("support_max_pillar_link_distance", supports_en && is_default_tree);
    toggle_field("support_pillar_widening_factor", false);
    toggle_field("support_max_weight_on_model", false);

    toggle_field("branchingsupport_head_front_diameter", supports_en && is_branching_tree);
    toggle_field("branchingsupport_head_penetration", supports_en && is_branching_tree);
    toggle_field("branchingsupport_head_width", supports_en && is_branching_tree);
    toggle_field("branchingsupport_pillar_diameter", supports_en && is_branching_tree);
    toggle_field("branchingsupport_small_pillar_diameter_percent", supports_en && is_branching_tree);
    toggle_field("branchingsupport_max_bridges_on_pillar", false);
    toggle_field("branchingsupport_pillar_connection_mode", false);
    toggle_field("branchingsupport_buildplate_only", supports_en && is_branching_tree);
    toggle_field("branchingsupport_base_diameter", supports_en && is_branching_tree);
    toggle_field("branchingsupport_base_height", supports_en && is_branching_tree);
    toggle_field("branchingsupport_base_safety_distance", supports_en && is_branching_tree);
    toggle_field("branchingsupport_critical_angle", supports_en && is_branching_tree);
    toggle_field("branchingsupport_max_bridge_length", supports_en && is_branching_tree);
    toggle_field("branchingsupport_max_pillar_link_distance", false);
    toggle_field("branchingsupport_pillar_widening_factor", supports_en && is_branching_tree);
    toggle_field("branchingsupport_max_weight_on_model", supports_en && is_branching_tree);

    toggle_field("support_points_density_relative", supports_en);

    bool pad_en = config->opt_bool("pad_enable");

    toggle_field("pad_wall_thickness", pad_en);
    toggle_field("pad_wall_height", pad_en);
    toggle_field("pad_brim_size", pad_en);
    toggle_field("pad_max_merge_distance", pad_en);
 // toggle_field("pad_edge_radius", supports_en);
    toggle_field("pad_wall_slope", pad_en);
    toggle_field("pad_around_object", pad_en);
    toggle_field("pad_around_object_everywhere", pad_en);

    bool zero_elev = config->opt_bool("pad_around_object") && pad_en;

    toggle_field("support_object_elevation", supports_en && is_default_tree && !zero_elev);
    toggle_field("branchingsupport_object_elevation", supports_en && is_branching_tree && !zero_elev);
    toggle_field("pad_object_gap", zero_elev);
    toggle_field("pad_around_object_everywhere", zero_elev);
    toggle_field("pad_object_connector_stride", zero_elev);
    toggle_field("pad_object_connector_width", zero_elev);
    toggle_field("pad_object_connector_penetration", zero_elev);
}


} // GUI
} // Slic3r
