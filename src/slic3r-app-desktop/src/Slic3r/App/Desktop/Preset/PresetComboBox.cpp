///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, Filip Sykala @Jony01, Lukáš Matěna @lukasmatena, Tomáš Mészáros @tamasmeszaros
///|/ Copyright (c) 2021 Scott Mudge @ScottMudge
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "PresetComboBox.hpp"
//#include "AbstractEditor.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/BitmapCache.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/I18N.hpp"
#include "Slic3r/App/WX/format.hpp"

#include <cstddef>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Color.hpp"

// A workaround for a set of issues related to text fitting into gtk widgets:
// See e.g.: https://github.com/prusa3d/PrusaSlicer/issues/4584
#if defined(__WXGTK20__) || defined(__WXGTK3__)
    #include <glib-2.0/glib-object.h>
    #include <pango-1.0/pango/pango-layout.h>
    #include <gtk/gtk.h>
#endif

//!#include "GUIApp.hpp"  // for -> open_browser_with_warning_dialog(), get_tab() and show_printer_webview_tab()
//!#include "Plater.hpp"  // for -> for update presets in sidebar 

namespace Slic3r::App::Desktop::Preset {

using WX::_;

/* For PresetComboBox we use bitmaps that are created from images that are already scaled appropriately for Retina
 * (Contrary to the intuition, the `scale` argument for Bitmap's constructor doesn't mean
 * "please scale this to such and such" but rather
 * "the wxImage is already sized for backing scale such and such". )
 * Unfortunately, the constructor changes the size of wxBitmap too.
 * Thus We need to use unscaled size value for bitmaps that we use
 * to avoid scaled size of control items.
 * For this purpose control drawing methods and
 * control size calculation methods (virtual) are overridden.
 **/

PresetComboBox::PresetComboBox(wxWindow* parent, Slic3r::Preset::Type preset_type, const wxSize& size, PresetBundle* preset_bundle/* = nullptr*/) :
    WX::Widgets::BitmapComboBox(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size, 0, nullptr, wxCB_READONLY),
    m_type(preset_type),
    m_last_selected(wxNOT_FOUND),
    m_em_unit(WX::w_config()->em_unit(this))
{
    init_from_bundle(preset_bundle);
    
    m_bitmapCompatible   = WX::get_bmp_bundle("flag_green");
    m_bitmapIncompatible = WX::get_bmp_bundle("flag_red");

    // parameters for an icon's drawing
    fill_width_height();

    Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
        if (m_suppress_change)
            e.StopPropagation();
        else
            e.Skip();
    });
    Bind(wxEVT_COMBOBOX_DROPDOWN, [this](wxCommandEvent&) { m_suppress_change = false; });
    Bind(wxEVT_COMBOBOX_CLOSEUP,  [this](wxCommandEvent&) { m_suppress_change = true;  });

    Bind(wxEVT_COMBOBOX, &PresetComboBox::OnSelect, this);
}

void PresetComboBox::init_from_bundle(PresetBundle* preset_bundle)
{
    if (!preset_bundle)
        return;
    m_preset_bundle = preset_bundle;
    assert(preset_bundle);

    switch (m_type)
    {
    case Slic3r::Preset::TYPE_PRINT: {
        m_collection = &m_preset_bundle->prints;
        m_main_bitmap_name = "cog";
        break;
    }
    case Slic3r::Preset::TYPE_FILAMENT: {
        m_collection = &m_preset_bundle->filaments;
        m_main_bitmap_name = "spool";
        break;
    }
    case Slic3r::Preset::TYPE_SLA_PRINT: {
        m_collection = &m_preset_bundle->sla_prints;
        m_main_bitmap_name = "cog";
        break;
    }
    case Slic3r::Preset::TYPE_SLA_MATERIAL: {
        m_collection = &m_preset_bundle->sla_materials;
        m_main_bitmap_name = "resin";
        break;
    }
    case Slic3r::Preset::TYPE_PRINTER: {
        m_collection = &m_preset_bundle->printers;
        m_main_bitmap_name = "printer";
        break;
    }
    default: break;
    }
}

void PresetComboBox::OnSelect(wxCommandEvent& evt)
{
    // see https://github.com/prusa3d/PrusaSlicer/issues/3889
    // Under OSX: in case of use of a same names written in different case (like "ENDER" and "Ender")
    // m_presets_choice->GetSelection() will return first item, because search in PopupListCtrl is case-insensitive.
    // So, use GetSelection() from event parameter 
    auto selected_item = evt.GetSelection();

    auto marker = reinterpret_cast<Marker>(this->GetClientData(selected_item));
    if (marker >= LABEL_ITEM_DISABLED && marker < LABEL_ITEM_MAX)
        this->SetSelection(m_last_selected);
    else if (on_selection_changed && (m_last_selected != selected_item || m_collection->current_is_dirty())) {
        m_last_selected = selected_item;
        on_selection_changed(selected_item);
        evt.StopPropagation();
    }
    evt.Skip();
}

PresetComboBox::~PresetComboBox()
{
}

WX::BitmapCache& PresetComboBox::bitmap_cache()
{
    static WX::BitmapCache bmps;
    return bmps;
}

bool PresetComboBox::allow_templates() const
{
    return false; //! >   !wxGetApp().app_config->get_bool("no_templates");
}

void PresetComboBox::set_label_marker(int item, LabelItemType label_item_type)
{
    this->SetClientData(item, (void*)label_item_type);
}
/*
bool PresetComboBox::set_printer_technology(PrinterTechnology pt)
{
    if (printer_technology != pt) {
        printer_technology = pt;
        return true;
    }
    return false;
}
*/
void PresetComboBox::invalidate_selection()
{
    m_last_selected = INT_MAX; // this value means that no one item is selected
}

void PresetComboBox::validate_selection(bool predicate/*=false*/)
{
    if (predicate ||
        // just in case: mark m_last_selected as a first added element
        m_last_selected == INT_MAX)
        m_last_selected = GetCount() - 1;
}

void PresetComboBox::update_selection()
{
    /* If selected_preset_item is still equal to INT_MAX, it means that
     * there is no presets added to the list.
     * So, select last combobox item ("Add/Remove preset")
     */
    validate_selection();

    SetSelection(m_last_selected);
#ifdef __WXMSW__
    // From the Windows 2004 the tooltip for preset combobox doesn't work after next call of SetTooltip()
    // (There was an issue, when tooltip doesn't appears after changing of the preset selection)
    // But this workaround seems to work: We should to kill tooltip and than set new tooltip value
    SetToolTip(NULL);
#endif
    SetToolTip(GetString(m_last_selected));

// A workaround for a set of issues related to text fitting into gtk widgets:
// See e.g.: https://github.com/prusa3d/PrusaSlicer/issues/4584
#if defined(__WXGTK20__) || defined(__WXGTK3__)
    GList* cells = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(m_widget));

    // 'cells' contains the GtkCellRendererPixBuf for the icon,
    // 'cells->next' contains GtkCellRendererText for the text we need to ellipsize
    if (!cells || !cells->next) return;

    auto cell = static_cast<GtkCellRendererText *>(cells->next->data);

    if (!cell) return;

    g_object_set(G_OBJECT(cell), "ellipsize", PANGO_ELLIPSIZE_END, (char*)NULL);

    // Only the list of cells must be freed, the renderer isn't ours to free
    g_list_free(cells);
#endif
}

std::string PresetComboBox::suffix(const Slic3r::Preset& preset)
{
    return (preset.is_dirty ? Slic3r::Preset::suffix_modified() : "");
}

std::string PresetComboBox::suffix(Slic3r::Preset* preset)
{
    return (preset->is_dirty ? Slic3r::Preset::suffix_modified() : "");
}

wxString PresetComboBox::get_preset_name(const Slic3r::Preset & preset)
{
    return WX::from_u8(preset.name);
}

static wxString get_preset_name_with_suffix(const Slic3r::Preset & preset)
{
    return WX::from_u8(preset.name + Slic3r::Preset::suffix_modified());
}

void PresetComboBox::update(std::string select_preset_name)
{
    Freeze();
    Clear();
    invalidate_selection();

    const ExtruderFilaments* extruder_filaments = m_preset_bundle->extruders_filaments.empty() ? nullptr : &m_preset_bundle->extruders_filaments[m_extruder_idx];

    const std::deque<Slic3r::Preset>& presets = m_collection->get_presets();

    std::vector<PresetData> system_presets;
    std::vector<PresetData> nonsys_presets;
    std::vector<PresetData> incomp_presets;
    std::vector<PresetData> template_presets;

    wxString selected = "";
    if (!presets.front().is_visible)
        set_label_marker(Append(separator(L("System presets")), NullBitmapBndl()));

    for (size_t i = presets.front().is_visible ? 0 : m_collection->num_default_presets(); i < presets.size(); ++i)
    {
        const Slic3r::Preset& preset = presets[i];
        const bool is_compatible = m_type == Slic3r::Preset::TYPE_FILAMENT && extruder_filaments ? extruder_filaments->filament(i).is_compatible : preset.is_compatible;

        if (!m_show_all && (!preset.is_visible || !is_compatible))
            continue;

        // marker used for disable incompatible printer models for the selected physical printer
        bool is_enabled = m_type == Slic3r::Preset::TYPE_PRINTER && m_printer_technology != ptAny ? preset.printer_technology() == m_printer_technology : true;
        if (select_preset_name.empty() && is_enabled)
            select_preset_name = preset.name;

        std::string   bitmap_key = "cb";
        if (m_type == Slic3r::Preset::TYPE_PRINTER) {
            bitmap_key += "_printer";
            if (preset.printer_technology() == ptSLA)
                bitmap_key += "_sla";
        }
        std::string main_icon_name = m_type == Slic3r::Preset::TYPE_PRINTER && preset.printer_technology() == ptSLA ? "sla_printer" : m_main_bitmap_name;

        auto bmp = get_bmp(bitmap_key, main_icon_name, "lock_closed", is_enabled, is_compatible, preset.is_system || preset.is_default);
        assert(bmp);

        if (!is_enabled) {
            incomp_presets.push_back({get_preset_name(preset), get_preset_name(preset).Lower(), bmp, false});
            if (preset.is_dirty && m_show_modif_preset_separately)
                incomp_presets.push_back({get_preset_name_with_suffix(preset), get_preset_name_with_suffix(preset).Lower(), bmp, false});
        }
        else if (preset.is_default || preset.is_system)
        {
            if (preset.vendor && preset.vendor->templates_profile) {
                if (allow_templates())
                    template_presets.push_back({ get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled });
            }
            else {
                system_presets.push_back({ get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled });
            }
            if (preset.name == select_preset_name)
                selected = preset.name;

            if (preset.is_dirty && m_show_modif_preset_separately) {
                wxString preset_name = get_preset_name_with_suffix(preset);
                if (preset.vendor && preset.vendor->templates_profile) {
                    if (allow_templates())
                        template_presets.push_back({ get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled });
                }
                else
                    system_presets.push_back({preset_name, preset_name.Lower(), bmp, is_enabled});
                if (WX::into_u8(preset_name) == select_preset_name)
                    selected = preset_name;
            }
        }
        else
        {
            nonsys_presets.push_back({get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled});
            if (preset.name == select_preset_name || (select_preset_name.empty() && is_enabled))
                selected = get_preset_name(preset);
            if (preset.is_dirty && m_show_modif_preset_separately) {
                wxString preset_name = get_preset_name_with_suffix(preset);
                nonsys_presets.push_back({preset_name, preset_name.Lower(), bmp, is_enabled});
                if (preset_name == select_preset_name || (select_preset_name.empty() && is_enabled))
                    selected = preset_name;
            }
        }
        if (i + 1 == m_collection->num_default_presets())
            set_label_marker(Append(separator(L("System presets")), NullBitmapBndl()));
    }
    
    if (!system_presets.empty())
    {
        std::sort(system_presets.begin(), system_presets.end(), [](const PresetData& a, const PresetData& b) {
            return a.lower_name < b.lower_name;
            });

        for (std::vector<PresetData>::iterator it = system_presets.begin(); it != system_presets.end(); ++it) {
            int item_id = Append(it->name, *it->bitmap);
            if (!it->enabled)
                set_label_marker(item_id, LABEL_ITEM_DISABLED);
            validate_selection(it->name == selected);
        }
    }
    if (!nonsys_presets.empty())
    {
        std::sort(nonsys_presets.begin(), nonsys_presets.end(), [](const PresetData& a, const PresetData& b) {
            return a.lower_name < b.lower_name;
            });

        set_label_marker(Append(separator(L("User presets")), NullBitmapBndl()));
        for (std::vector<PresetData>::iterator it = nonsys_presets.begin(); it != nonsys_presets.end(); ++it) {
            int item_id = Append(it->name, *it->bitmap);
            if (!it->enabled)
                set_label_marker(item_id, LABEL_ITEM_DISABLED);
            validate_selection(it->name == selected);
        }
    }

    if (!template_presets.empty())
    {
        std::sort(template_presets.begin(), template_presets.end(), [](const PresetData& a, const PresetData& b) {
            return a.lower_name < b.lower_name;
            });

        set_label_marker(Append(separator(L("Template presets")), wxNullBitmap));
        for (std::vector<PresetData>::iterator it = template_presets.begin(); it != template_presets.end(); ++it) {
            int item_id = Append(it->name, *it->bitmap);
            if (!it->enabled)
                set_label_marker(item_id, LABEL_ITEM_DISABLED);
            validate_selection(it->name == selected);
        }
    }

    if (!incomp_presets.empty())
    {
        std::sort(incomp_presets.begin(), incomp_presets.end(), [](const PresetData& a, const PresetData& b) {
            return a.lower_name < b.lower_name;
            });

        set_label_marker(Append(separator(L("Incompatible presets")), NullBitmapBndl()));
        for (std::vector<PresetData>  ::iterator it = incomp_presets.begin(); it != incomp_presets.end(); ++it) {
            set_label_marker(Append(it->name, *it->bitmap), LABEL_ITEM_DISABLED);
        }
    }
    
    update_selection();
    Thaw();
}
/* //!
void PresetComboBox::open_physical_printer_url()
{
    const PhysicalPrinter& pp = m_preset_bundle->physical_printers.get_selected_printer();
    std::string host = pp.config.opt_string("print_host");
    assert(!host.empty());
    wxGetApp().open_browser_with_warning_dialog(host);
}
*/

void PresetComboBox::show_all(bool show_all)
{
    m_show_all = show_all;
    update();
}

void PresetComboBox::update()
{
    int n = this->GetSelection();
    this->update(n < 0 ? "" : WX::into_u8(this->GetString(n)));
}

void PresetComboBox::update_from_bundle()
{
    if (m_collection->type() == Slic3r::Preset::TYPE_FILAMENT && !m_preset_bundle->extruders_filaments.empty())
        this->update(m_preset_bundle->extruders_filaments[m_extruder_idx].get_selected_preset_name());
    else
        this->update(m_collection->get_selected_preset().name);
}

void PresetComboBox::msw_rescale()
{
    m_em_unit = WX::w_config()->em_unit(this);
    ComboBox::Rescale();
}

void PresetComboBox::sys_color_changed()
{
    m_bitmapCompatible = WX::get_bmp_bundle("flag_green");
    m_bitmapIncompatible = WX::get_bmp_bundle("flag_red");
    WX::w_config()->UpdateDarkUI(this);

    // update the control to redraw the icons
    update();
}

void PresetComboBox::fill_width_height()
{
    m_icon_height     = 16;
    m_norm_icon_width = 16;

    m_thin_icon_width = 8;
    m_wide_icon_width = m_norm_icon_width + m_thin_icon_width;

    m_null_icon_width = 2 * m_norm_icon_width;

    m_space_icon_width      = 2;
    m_thin_space_icon_width = 4;
    m_wide_space_icon_width = 6;
}

wxString PresetComboBox::separator(const std::string& label)
{
    return WX::format_wxstr("%1%%2%%3%", separator_head(), _(label), separator_tail());
}


wxBitmapBundle* PresetComboBox::get_bmp(  std::string bitmap_key, bool wide_icons, const std::string& main_icon_name,
                                    bool is_compatible/* = true*/, bool is_system/* = false*/, bool is_single_bar/* = false*/,
                                    const std::string& filament_rgb/* = ""*/, const std::string& extruder_rgb/* = ""*/, const std::string& material_rgb/* = ""*/)
{
    // If the filament preset is not compatible and there is a "red flag" icon loaded, show it left
    // to the filament color image.
    if (wide_icons)
        bitmap_key += is_compatible ? ",cmpt" : ",ncmpt";

    bitmap_key += is_system ? ",syst" : ",nsyst";
    bitmap_key += ",h" + std::to_string(m_icon_height);
    bool dark_mode = WX::w_config()->dark_mode();
    if (dark_mode)
        bitmap_key += ",dark";
    bitmap_key += material_rgb;

    wxBitmapBundle* bmp_bndl = bitmap_cache().find_bndl(bitmap_key);
    if (bmp_bndl == nullptr) {
        // Create the bitmap with color bars.
        std::vector<wxBitmapBundle*> bmps;
        if (wide_icons)
            // Paint a red flag for incompatible presets.
            bmps.emplace_back(is_compatible ? WX::get_empty_bmp_bundle(m_norm_icon_width, m_icon_height) : m_bitmapIncompatible);

        if (m_type == Slic3r::Preset::TYPE_FILAMENT && !filament_rgb.empty()) {
            // Paint the color bars.
            bmps.emplace_back(WX::get_solid_bmp_bundle(is_single_bar ? m_wide_icon_width : m_norm_icon_width, m_icon_height, filament_rgb));
            if (!is_single_bar)
                bmps.emplace_back(WX::get_solid_bmp_bundle(m_thin_icon_width, m_icon_height, extruder_rgb));
            // Paint a lock at the system presets.
            bmps.emplace_back(WX::get_empty_bmp_bundle(m_space_icon_width, m_icon_height));
        }
        else
        {
            // Paint the color bars.
            bmps.emplace_back(WX::get_empty_bmp_bundle(m_thin_space_icon_width, m_icon_height));
            if (m_type == Slic3r::Preset::TYPE_SLA_MATERIAL)
                bmps.emplace_back(bitmap_cache().from_svg(main_icon_name, 16, 16, dark_mode, material_rgb));
            else
                bmps.emplace_back(WX::get_bmp_bundle(main_icon_name));
            // Paint a lock at the system presets.
            bmps.emplace_back(WX::get_empty_bmp_bundle(m_wide_space_icon_width, m_icon_height));
        }
        bmps.emplace_back(is_system ? WX::get_bmp_bundle("lock_closed") : WX::get_empty_bmp_bundle(m_norm_icon_width, m_icon_height));
        bmp_bndl = bitmap_cache().insert_bndl(bitmap_key, bmps);
    }

    return bmp_bndl;
}

wxBitmapBundle* PresetComboBox::get_bmp(  std::string bitmap_key, const std::string& main_icon_name, const std::string& next_icon_name,
                                    bool is_enabled/* = true*/, bool is_compatible/* = true*/, bool is_system/* = false*/)
{
    bitmap_key += !is_enabled ? "_disabled" : "";
    bitmap_key += is_compatible ? ",cmpt" : ",ncmpt";
    bitmap_key += is_system ? ",syst" : ",nsyst";
    bitmap_key += ",h" + std::to_string(m_icon_height);
    if (WX::w_config()->dark_mode())
        bitmap_key += ",dark";

    wxBitmapBundle* bmp = bitmap_cache().find_bndl(bitmap_key);
    if (bmp == nullptr) {
        // Create the bitmap with color bars.
        std::vector<wxBitmapBundle*> bmps;
        bmps.emplace_back(m_type == Slic3r::Preset::TYPE_PRINTER ? WX::get_bmp_bundle(main_icon_name) :
                          is_compatible ? m_bitmapCompatible : m_bitmapIncompatible);
        // Paint a lock at the system presets.
        bmps.emplace_back(is_system ? WX::get_bmp_bundle(next_icon_name) : WX::get_empty_bmp_bundle(m_norm_icon_width, m_icon_height));
        bmp = bitmap_cache().insert_bndl(bitmap_key, bmps);
    }

    return bmp;
}

wxBitmapBundle PresetComboBox::NullBitmapBndl()
{
    assert(m_null_icon_width > 0);
    return *WX::get_empty_bmp_bundle(m_null_icon_width, m_icon_height);
}

bool PresetComboBox::is_selected_physical_printer()
{
    auto selected_item = this->GetSelection();
    auto marker = reinterpret_cast<Marker>(this->GetClientData(selected_item));
    return marker == LABEL_ITEM_PHYSICAL_PRINTER;
}
/*
bool PresetComboBox::selection_is_changed_according_to_physical_printers()
{
    if (m_type != Preset::TYPE_PRINTER)
        return false;

    const std::string           selected_string     = WX::into_u8(this->GetString(this->GetSelection()));
    PhysicalPrinterCollection&  physical_printers   = m_preset_bundle->physical_printers;
    Tab*                        tab                 = nullptr; //! wxGetApp().get_tab(Preset::TYPE_PRINTER);

    if (!is_selected_physical_printer()) {
        if (!physical_printers.has_selection())
            return false;

        const bool is_changed = selected_string == physical_printers.get_selected_printer_preset_name();
        physical_printers.unselect_printer();
        if (is_changed)
            tab->select_preset(selected_string);
        return is_changed;
    }

    std::string old_printer_full_name, old_printer_preset;
    if (physical_printers.has_selection()) {
        old_printer_full_name = physical_printers.get_selected_full_printer_name();
        old_printer_preset = physical_printers.get_selected_printer_preset_name();
    }
    else
        old_printer_preset = m_collection->get_edited_preset().name;
    // Select related printer preset on the Printer Settings Tab 
    physical_printers.select_printer(selected_string);
    std::string preset_name = physical_printers.get_selected_printer_preset_name();

    // if new preset wasn't selected, there is no need to call update preset selection
    if (old_printer_preset == preset_name) {
        tab->update_preset_choice();
        // update action buttons to show/hide "Send to" button
        wxGetApp().plater()->show_action_buttons();

        // we need just to update according Plater<->Tab PresetComboBox 
        if (dynamic_cast<PlaterPresetComboBox*>(this)!=nullptr) {
            // Synchronize config.ini with the current selections.
            m_preset_bundle->export_selections(*wxGetApp().app_config);
            this->update();
        }
        else if (dynamic_cast<EditorPresetComboBox*>(this)!=nullptr)
            wxGetApp().sidebar().update_presets(m_type);

        // Check and show "Physical printer" page if needed
        wxGetApp().show_printer_webview_tab();
        return true;
    }

    if (tab)
        tab->select_preset(preset_name, false, old_printer_full_name);
    return true;
}
*/
}
