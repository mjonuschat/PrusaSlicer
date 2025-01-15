///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2021 Martin Budden
///|/ Copyright (c) 2021 Ilya @xorza
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2019 Matthias Urlichs @smurfix
///|/ Copyright (c) 2019 Thomas Moore
///|/ Copyright (c) 2019 Sijmen Schoon
///|/ Copyright (c) 2018 Martin Loidl @LoidlM
///|/
///|/ ported from lib/Slic3r/GUI/Tab.pm:
///|/ Copyright (c) Prusa Research 2016 - 2018 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/ Copyright (c) 2015 - 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2016 Chow Loong Jin @hyperair
///|/ Copyright (c) 2012 QuantumConcepts
///|/ Copyright (c) 2012 Henrik Brix Andersen @henrikbrixandersen
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "EditorSLAMaterial.hpp"
#include "../Config/OptionsGroup.hpp"

#include "Slic3r/App/WX/format.hpp"

#include "Slic3r/App/WX/Widgets/CheckBox.hpp"
#include "Slic3r/App/WX/I18N.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include <wx/bmpbuttn.h>
#include <wx/wupdlock.h>

namespace Slic3r::App::Desktop::Preset {

using WX::_L;

EditorSLAMaterial::EditorSLAMaterial(wxWindow* parent, Biz::Preset::PresetInteractor& preset_interactor) :
    AbstractEditor(parent, _L("Materials"), Slic3r::Preset::TYPE_SLA_MATERIAL, preset_interactor)
{
    m_config_interactor = std::make_unique<Biz::Preset::PresetConfigInteractor>(preset_interactor, Slic3r::Preset::TYPE_SLA_MATERIAL, 0);
}

void EditorSLAMaterial::build()
{
    load_initial_data();

    auto page = add_options_page(_L("Material"), "resin");

    auto optgroup = page->new_optgroup(_L("Material"));
    optgroup->append_single_option_line("material_colour");
    optgroup->append_single_option_line("bottle_cost");
    optgroup->append_single_option_line("bottle_volume");
    optgroup->append_single_option_line("bottle_weight");
    optgroup->append_single_option_line("material_density");

    optgroup->on_change = [this](t_config_option_key opt_key, boost::any value)
    {
        if (opt_key == "material_colour") {
            update_dirty();
            on_value_change(opt_key, value); 
            return;
        }

        DynamicPrintConfig new_conf = config();

        if (opt_key == "bottle_volume") {
            double new_bottle_weight =  boost::any_cast<double>(value)*(new_conf.option("material_density")->getFloat() / 1000);
            new_conf.set_key_value("bottle_weight", new ConfigOptionFloat(new_bottle_weight));
        }
        if (opt_key == "bottle_weight") {
            double new_bottle_volume =  boost::any_cast<double>(value)/new_conf.option("material_density")->getFloat() * 1000;
            new_conf.set_key_value("bottle_volume", new ConfigOptionFloat(new_bottle_volume));
        }
        if (opt_key == "material_density") {
            double new_bottle_volume = new_conf.option("bottle_weight")->getFloat() / boost::any_cast<double>(value) * 1000;
            new_conf.set_key_value("bottle_volume", new ConfigOptionFloat(new_bottle_volume));
        }

        load_config(new_conf);

        update_dirty();
        /*  //!
        // Change of any from those options influences for an update of "Sliced Info"
        wxGetApp().sidebar().update_sliced_info_sizer();
        wxGetApp().sidebar().Layout();
*/
    };

    optgroup = page->new_optgroup(_L("Layers"));
    optgroup->append_single_option_line("initial_layer_height");

    optgroup = page->new_optgroup(_L("Exposure"));
    optgroup->append_single_option_line("exposure_time");
    optgroup->append_single_option_line("initial_exposure_time");

    optgroup = page->new_optgroup(_L("Corrections"));
    auto line = Line{ WX::from_u8(config().def()->get("material_correction")->full_label), {} };
    for (auto& axis : { "X", "Y", "Z" }) {
        auto opt = optgroup->get_option(std::string("material_correction_") + char(std::tolower(axis[0])));
        opt.opt.label = axis;
        line.append_option(opt);
    }
    optgroup->append_line(line);

    optgroup->append_single_option_line("zcorrection_layers");

    line = Line{ {}, {} };
    line.full_width = 1;
    // line.label_path = category_path + "recommended-thin-wall-thickness";
    line.widget = [this](wxWindow* parent) {
        return description_line_widget(parent, &m_z_correction_to_mm_description);
    };
    optgroup->append_line(line);

    add_material_overrides_page();

    page = add_options_page(_L("Notes"), "note");
    optgroup = page->new_optgroup(_L("Notes"), 0);
    optgroup->label_width = 0;
    Option option = optgroup->get_option("material_notes");
    option.opt.full_width = true;
    option.opt.height = 25;//250;
    optgroup->append_single_option_line(option);

    page = add_options_page(_L("Dependencies"), "wrench");
    optgroup = page->new_optgroup(_L("Profile dependencies"));

    create_line_with_widget(optgroup.get(), "compatible_printers", "", [this](wxWindow* parent) {
        return compatible_widget_create(parent, m_compatible_printers);
    });
    
    option = optgroup->get_option("compatible_printers_condition");
    option.opt.full_width = true;
    optgroup->append_single_option_line(option);

    create_line_with_widget(optgroup.get(), "compatible_prints", "", [this](wxWindow* parent) {
        return compatible_widget_create(parent, m_compatible_prints);
    });

    option = optgroup->get_option("compatible_prints_condition");
    option.opt.full_width = true;
    optgroup->append_single_option_line(option);

    build_preset_description_line(optgroup.get());

    page = add_options_page(_L("Material printing profile"), "note");

#if 1
    optgroup = page->new_optgroup(_L("Material printing profile"));
    optgroup->append_single_option_line("material_print_speed");

    optgroup = page->new_optgroup(_L("Tilt"));
    optgroup->append_single_option_line("area_fill");

#else
    optgroup = page->new_optgroup(L("Material printing profile"));
    option = optgroup->get_option("material_print_speed");
    optgroup->append_single_option_line(option);

    optgroup->append_single_option_line("area_fill");
#endif

    build_tilt_group(page);
}

static void append_tilt_options_line(ConfigOptionsGroupShp optgroup, const std::string opt_key)
{
    auto option = optgroup->get_option(opt_key, 0);
    auto line = Line{ WX::from_u8(option.opt.full_label), {} };
    option.opt.width = Field::def_width/*_wider*/();
    line.append_option(option);

    option = optgroup->get_option(opt_key, 1);
    option.opt.width = Field::def_width/*_wider*/();
    line.append_option(option);

    optgroup->append_line(line);
}

void EditorSLAMaterial::build_tilt_group(PageShp page)
{
    // Legend
    std::vector<std::pair<std::string, std::string>> legend_columns = {
        // TRN: This is a label of a column of parameters in settings to be used when the area is below certain threshold.
        {L("Below"),
        L("Values in this column are applied when layer area is smaller than area_fill.")},
        // TRN: This is a label of a column of parameters in settings to be used when the area is above certain threshold.
        {L("Above"),
        L("Values in this column are applied when layer area is larger than area_fill.")},
    };
    create_legend(page, legend_columns, comExpert/*, true*/);

    // TRN: 'Profile' in this context denotes a group of parameters used to configure
    //      layer separation procedure for SLA printers.
    auto optgroup = page->new_optgroup(_L("Profile settings"));
    optgroup->on_change = [this, optgroup](const t_config_option_key& key, boost::any value)
    {
        if (key.find_first_of("use_tilt") == 0)
            toggle_tilt_options(key == "use_tilt#0");

        update_dirty();
        update();
    };

    for (const std::string& opt_key : tilt_options())
        append_tilt_options_line(optgroup, opt_key);
}

std::vector<std::string> disable_tilt_options = {
         "tilt_down_initial_speed"
        ,"tilt_down_offset_steps"
        ,"tilt_down_offset_delay"
        ,"tilt_down_finish_speed"
        ,"tilt_down_cycles"
        ,"tilt_down_delay"
        ,"tilt_up_initial_speed"
        ,"tilt_up_offset_steps"
        ,"tilt_up_offset_delay"
        ,"tilt_up_finish_speed"
        ,"tilt_up_cycles"
        ,"tilt_up_delay"
};

void EditorSLAMaterial::toggle_tilt_options(bool is_above)
{
    if (m_active_page && m_active_page->title() == WX::from_u8("Material printing profile"))
    {
        int column_id = is_above ? 0 : 1;
        auto optgroup = m_active_page->get_optgroup(WX::from_u8("Profile settings"));
        bool use_tilt = boost::any_cast<bool>(optgroup->get_config_value(config(), "use_tilt", column_id));

        for (const std::string& opt_key : disable_tilt_options) {
            auto field = optgroup->get_fieldc(opt_key, column_id);
            if (field != nullptr)
                field->toggle(use_tilt);
        }
    }
}

void EditorSLAMaterial::toggle_options()
{
    if (m_active_page->title() == WX::from_u8("Material Overrides"))
        update_material_overrides_page();
}

void EditorSLAMaterial::update()
{
    toggle_options();

    update_description_lines();
    Layout();

//!    wxGetApp().mainframe->on_config_changed(m_config);
}

void EditorSLAMaterial::update_description_lines()
{
    if (m_active_page && m_active_page->title() == WX::from_u8("Material") &&  m_z_correction_to_mm_description) {
        const auto& ccc = m_preset_interactor.selected_config_container_context();
        double lh = ccc.print.edited_preset.config.opt_float("layer_height");
        int zlayers = config().opt_int("zcorrection_layers");
        m_z_correction_to_mm_description->SetText(WX::format_wxstr(_L("The current Z-axis height correction is: %1% mm"), zlayers * lh));
    }

    AbstractEditor::update_description_lines();
}

void EditorSLAMaterial::update_sla_prusa_specific_visibility()
{
    if (m_active_page && m_active_page->title() == WX::from_u8("Material printing profile")) {
        for (auto& title : { "", "Profile settings" }) {
            auto og_it = std::find_if(m_active_page->optgroups.begin(), m_active_page->optgroups.end(), 
                         [title](const ConfigOptionsGroupShp og) { return og->title == WX::from_u8(title); });
            if (og_it != m_active_page->optgroups.end())
                og_it->get()->Show(m_mode >= comAdvanced && is_prusa_printer());
        }

        auto og_it = std::find_if(m_active_page->optgroups.begin(), m_active_page->optgroups.end(), 
                        [](const ConfigOptionsGroupShp og) { return og->title == WX::from_u8("Material printing profile"); });
        if (og_it != m_active_page->optgroups.end())
            og_it->get()->Show(m_mode >= comAdvanced && !is_prusa_printer());

        Layout();
    }
}

void EditorSLAMaterial::clear_pages()
{
    AbstractEditor::clear_pages();

    for (auto& over_opt : m_overrides_options)
        over_opt.second = nullptr;

    m_z_correction_to_mm_description = nullptr;
}

void EditorSLAMaterial::msw_rescale()
{
    for (const auto& over_opt : m_overrides_options)
        if (wxWindow* win = over_opt.second)
            win->SetInitialSize(win->GetBestSize());
    AbstractEditor::msw_rescale();
}

void EditorSLAMaterial::sys_color_changed()
{
    AbstractEditor::sys_color_changed();

    for (const auto& over_opt : m_overrides_options)
        if (wxWindow* check_box = over_opt.second) {
            WX::w_config()->UpdateDarkUI(check_box);
            CheckBox::SysColorChanged(check_box);
        }
}

static std::vector<std::string> get_override_opt_kyes_for_line(const std::string& title, const std::string& key)
{
    const std::string preprefix = "material_ow_";

    std::vector<std::string> opt_keys;
    opt_keys.reserve(3);

    if (title == "Support head" || title == "Support pillar") {
        for (auto& prefix : { "", "branching" })
            opt_keys.push_back(preprefix + prefix + key);
    }
    else
        opt_keys.push_back(preprefix + key);

    return opt_keys;
}

void EditorSLAMaterial::create_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& key)
{
    if (optgroup->title == WX::from_u8("Support head") || optgroup->title == WX::from_u8("Support pillar"))
        add_options_into_line(optgroup, { {"", L("Default")}, {"branching", L("Branching")} }, key, "material_ow_");
    else {
        const std::string opt_key = std::string("material_ow_") + key;
        optgroup->append_single_option_line(opt_key);
    }

    Line* line = optgroup->get_last_line();
    if (!line)
        return;

    line->near_label_widget = [this, optgroup_wk = ConfigOptionsGroupWkp(optgroup), key](wxWindow* parent) {
        wxWindow* check_box = CheckBox::GetNewWin(parent);
        WX::w_config()->UpdateDarkUI(check_box);

        check_box->Bind(wxEVT_CHECKBOX, [this, optgroup_wk, key](wxCommandEvent& evt) {
            const bool is_checked = evt.IsChecked();
            if (auto optgroup_sh = optgroup_wk.lock(); optgroup_sh) {
                auto opt_keys = get_override_opt_kyes_for_line(WX::into_u8(optgroup_sh->title), key);
                for (const std::string& opt_key : opt_keys)
                    if (Field* field = optgroup_sh->get_fieldc(opt_key, 0); field != nullptr) {
                        field->toggle(is_checked);
                        if (is_checked)
                            field->set_last_meaningful_value();
                        else
                            field->set_na_value();
                    }
            }

            toggle_options();
        });

        m_overrides_options[key] = check_box;
        return check_box;
    };
}

std::vector<std::pair<std::string, std::vector<std::string>>> material_overrides_option_keys{
    {"Support head", {
        "support_head_front_diameter",
        "support_head_penetration",
        "support_head_width"
    }},
    {"Support pillar", {
        "support_pillar_diameter",
    }},
    {"Automatic generation", {
        "support_points_density_relative"
    }},
    {"Corrections", {
        "absolute_correction",
        "elefant_foot_compensation"
    }}
};

void EditorSLAMaterial::add_material_overrides_page()
{
    // TRN: Page title in Material Settings in SLA mode.
    PageShp page = add_options_page(_L("Material Overrides"), "wrench");

    for (const auto& [title, keys] : material_overrides_option_keys) {
        ConfigOptionsGroupShp optgroup = page->new_optgroup(_L(title));
        for (const std::string& opt_key : keys) {
            create_line_with_near_label_widget(optgroup, opt_key);
        }
    }
}

void EditorSLAMaterial::update_line_with_near_label_widget(ConfigOptionsGroupShp optgroup, const std::string& key, bool is_checked/* = true*/)
{
    if (!m_overrides_options[key])
        return;

    const std::string preprefix = "material_ow_";

    std::vector<std::string> opt_keys;
    opt_keys.reserve(3);

    if (optgroup->title == WX::from_u8("Support head") || optgroup->title == WX::from_u8("Support pillar")) {
        for (auto& prefix : { "", "branching" }) {
            std::string opt_key = preprefix + prefix + key;
            is_checked = !config().option(opt_key)->is_nil();
            opt_keys.push_back(opt_key);
        }
    }
    else if (key == "relative_correction") {
        for (auto& axis : { "x", "y", "z" }) {
            std::string opt_key = preprefix + key + "_" + char(axis[0]);
            is_checked = !config().option(opt_key)->is_nil();
            opt_keys.push_back(opt_key);
        }
    }
    else {
        std::string opt_key = preprefix + key;
        is_checked = !config().option(opt_key)->is_nil();
        opt_keys.push_back(opt_key);
    }

    CheckBox::SetValue(m_overrides_options[key], is_checked);

    for (const std::string& opt_key : opt_keys) {
        Field* field = optgroup->get_field(opt_key);
        if (field != nullptr)
            field->toggle(is_checked);
    }
}

void EditorSLAMaterial::update_material_overrides_page()
{
    if (!m_active_page || m_active_page->title() != WX::from_u8("Material Overrides"))
            return;
    Page* page = m_active_page;

    for (const auto& [title, keys] : material_overrides_option_keys) {
        std::optional<ConfigOptionsGroupShp> optgroup{ get_option_group(page, title) };
        if (!optgroup) {
            continue;
        }

        for (const std::string& key : keys) {
            update_line_with_near_label_widget(*optgroup, key);
        }
    }
}

} 
