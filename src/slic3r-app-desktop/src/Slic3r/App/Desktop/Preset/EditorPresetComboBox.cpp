///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, Filip Sykala @Jony01, Lukáš Matěna @lukasmatena, Tomáš Mészáros @tamasmeszaros
///|/ Copyright (c) 2021 Scott Mudge @ScottMudge
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "EditorPresetComboBox.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/BitmapCache.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"

#include "Slic3r/Biz/Preset/PresetState.hpp"
#include "Slic3r/Biz/Preset/PresetBundleRuntime.hpp"

#include <cstddef>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

#include <wx/wupdlock.h>

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

#include "Slic3r/App/I18N/I18N.hpp"

namespace Slic3r::App::Desktop::Preset {

EditorPresetComboBox::EditorPresetComboBox(wxWindow* parent, Slic3r::Preset::Type preset_type, const PresetBundle* preset_bundle) :
    PresetComboBox(parent, preset_type, wxSize(35 * WX::w_config()->em_unit(), -1), preset_bundle)
{
    /*
    set_selection_changed_function([this](int selection) {
        if (!selection_is_changed_according_to_physical_printers())
        {
            if (m_type == Slic3r::Preset::TYPE_PRINTER && !m_presets_choice->is_selected_physical_printer())
                m_preset_bundle->physical_printers.unselect_printer();

            // select preset
            std::string preset_name = m_presets_choice->GetString(selection).ToUTF8().data();
            select_preset(Preset::remove_suffix_modified(preset_name));
        }
    });
*/
}

void EditorPresetComboBox::OnSelect(wxCommandEvent &evt)
{
    // see https://github.com/prusa3d/PrusaSlicer/issues/3889
    // Under OSX: in case of use of a same names written in different case (like "ENDER" and "Ender")
    // m_presets_choice->GetSelection() will return first item, because search in PopupListCtrl is case-insensitive.
    // So, use GetSelection() from event parameter 
    auto selected_item = evt.GetSelection();

    auto marker = reinterpret_cast<Marker>(this->GetClientData(selected_item));
    if (marker >= LABEL_ITEM_DISABLED && marker < LABEL_ITEM_MAX) {
        this->SetSelection(m_last_selected);
        if (marker == LABEL_ITEM_WIZARD_PRINTERS)
            wxTheApp->CallAfter([this]() {
//!            run_wizard(ConfigWizard::SP_PRINTERS);
        });
    }
    else if (on_selection_changed && (m_last_selected != selected_item || m_collection->current_is_dirty())) {
        m_last_selected = selected_item;
        on_selection_changed(selected_item);
    }

    evt.StopPropagation();
#ifdef __WXMSW__
    // From the Win 2004 preset combobox lose a focus after change the preset selection
    // and that is why the up/down arrow doesn't work properly
    // (see https://github.com/prusa3d/PrusaSlicer/issues/5531 ).
    // So, set the focus to the combobox explicitly
    this->SetFocus();
#endif
}

wxString EditorPresetComboBox::get_preset_name(const Slic3r::Preset& preset)
{
    return WX::from_u8(preset.name + suffix(preset));
}

void EditorPresetComboBox::update(const Biz::Preset::PresetState* state, const Biz::Preset::PresetBundleRuntime* pb_runtime)
{
    m_preset_state = state;
    m_pb_runtime = pb_runtime;
    update();
}

// Update the choice UI from the list of presets.
// If m_show_incompatible, all presets are shown, otherwise only the compatible presets are shown.
// If an incompatible preset is selected, it is shown as well.
void EditorPresetComboBox::update()
{
    if (!m_preset_bundle)   //!
        return;

    Freeze();
    Clear();
    invalidate_selection();

    const ExtruderFilaments& extruder_filaments = m_preset_bundle->extruders_filaments[m_extruder_idx];

    const std::deque<Slic3r::Preset>& presets = m_collection->get_presets();
    
    std::vector<PresetData> system_presets;
    std::vector<PresetData> nonsys_presets;
    std::vector<PresetData> template_presets;

    wxString selected = "";
    if (!presets.front().is_visible)
        set_label_marker(Append(separator(L("System presets")), NullBitmapBndl()));
    size_t idx_selected = m_type == Slic3r::Preset::TYPE_FILAMENT ? extruder_filaments.get_selected_idx() : m_collection->get_selected_idx();

    if (m_type == Slic3r::Preset::TYPE_PRINTER && m_preset_bundle->physical_printers.has_selection()) {
        std::string sel_preset_name = m_preset_bundle->physical_printers.get_selected_printer_preset_name();
        const Slic3r::Preset* preset = m_collection->find_preset(sel_preset_name);
//        if (!preset || m_collection->get_selected_preset_name() != sel_preset_name)
//            m_preset_bundle->physical_printers.unselect_printer();
    }

    for (size_t i = presets.front().is_visible ? 0 : m_collection->num_default_presets(); i < presets.size(); ++i)
    {
        const Slic3r::Preset& preset = presets[i];

        const bool is_compatible = m_type == Slic3r::Preset::TYPE_FILAMENT ? extruder_filaments.filament(i).is_compatible : preset.is_compatible;

        if (!preset.is_visible || (!m_show_incompatible && !is_compatible && i != idx_selected))
            continue;
        
        // marker used for disable incompatible printer models for the selected physical printer
        bool is_enabled = true;

        std::string bitmap_key = "tab";
        if (m_type == Slic3r::Preset::TYPE_PRINTER) {
            bitmap_key += "_printer";
            if (preset.printer_technology() == ptSLA)
                bitmap_key += "_sla";
        }
        std::string main_icon_name = m_type == Slic3r::Preset::TYPE_PRINTER && preset.printer_technology() == ptSLA ? "sla_printer" : m_main_bitmap_name;

        auto bmp = get_bmp(bitmap_key, main_icon_name, "lock_closed", is_enabled, is_compatible, preset.is_system || preset.is_default);
        assert(bmp);

        if (preset.is_default || preset.is_system) {
            if (preset.vendor && preset.vendor->templates_profile) {
                if (allow_templates()) {
                    template_presets.push_back({get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled});
                    if (i == idx_selected)
                        selected = get_preset_name(preset);
                }
            } else {
                system_presets.push_back({get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled});
                if (i == idx_selected)
                    selected = get_preset_name(preset);
            }
        }
        else
        {
            std::pair<wxBitmapBundle*, bool> pair(bmp, is_enabled);
            nonsys_presets.push_back({get_preset_name(preset), get_preset_name(preset).Lower(), bmp, is_enabled});
            if (i == idx_selected)
                selected = get_preset_name(preset);
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
    
    if (m_type == Slic3r::Preset::TYPE_PRINTER)
    {
        // add Physical printers, if any exists
        if (!m_preset_bundle->physical_printers.empty()) {
            set_label_marker(Append(separator(L("Physical printers")), NullBitmapBndl()));
            const PhysicalPrinterCollection& ph_printers = m_preset_bundle->physical_printers;

            // Sort Physical printers in preset_data vector and than Append it in correct order
            struct PhysicalPrinterPresetData
            {
                wxString lower_name; // just for sorting
                std::string name; // preset_name
                std::string fullname; // full name
                bool selected; // is selected
            };
            std::vector<PhysicalPrinterPresetData> preset_data;
            for (PhysicalPrinterCollection::ConstIterator it = ph_printers.begin(); it != ph_printers.end(); ++it) {
                for (const std::string& preset_name : it->get_preset_names()) {
                    preset_data.push_back({wxString::FromUTF8(it->get_full_name(preset_name)).Lower(), preset_name, it->get_full_name(preset_name), ph_printers.is_selected(it, preset_name)});
                }
            }
            std::sort(preset_data.begin(), preset_data.end(), [](const PhysicalPrinterPresetData& a, const PhysicalPrinterPresetData& b) {
                return a.lower_name < b.lower_name;
                });
            for (const PhysicalPrinterPresetData& data : preset_data)
            {
                const Slic3r::Preset* preset = m_collection->find_preset(data.name);
                if (!preset || !preset->is_visible)
                    continue;
                std::string main_icon_name = preset->printer_technology() == ptSLA ? "sla_printer" : m_main_bitmap_name;

                auto bmp = get_bmp(main_icon_name, main_icon_name, "", true, true, false);
                assert(bmp);

                set_label_marker(Append(WX::from_u8(data.fullname + suffix(preset)), *bmp), LABEL_ITEM_PHYSICAL_PRINTER);
                validate_selection(data.selected);
            }
        }

        // add "Add/Remove printers" item
        std::string icon_name = "edit_uni";
        auto bmp = get_bmp("edit_preset_list, tab,", icon_name, "");
        assert(bmp);

        set_label_marker(Append(separator(L("Add/Remove printers")), *bmp), LABEL_ITEM_WIZARD_PRINTERS);
    }

    update_selection();
    Thaw();
}

void EditorPresetComboBox::msw_rescale()
{
    PresetComboBox::msw_rescale();
    wxSize sz = wxSize(35 * m_em_unit, -1);
    SetMinSize(sz);
    SetSize(sz);
}

void EditorPresetComboBox::update_dirty()
{
    // 1) Update the dirty flag of the current preset.
    //RMV m_collection->update_dirty();

    // 2) Update the labels.
    wxWindowUpdateLocker noUpdates(this);
    for (unsigned int ui_id = 0; ui_id < GetCount(); ++ui_id) {
        auto marker = reinterpret_cast<Marker>(this->GetClientData(ui_id));
        if (marker >= LABEL_ITEM_MARKER)
            continue;

        std::string   old_label = GetString(ui_id).utf8_str().data();
        std::string   preset_name = Slic3r::Preset::remove_suffix_modified(old_label);
        std::string   ph_printer_name;

        if (marker == LABEL_ITEM_PHYSICAL_PRINTER) {
            ph_printer_name = PhysicalPrinter::get_short_name(preset_name);
            preset_name = PhysicalPrinter::get_preset_name(preset_name);
        }
            
        const Slic3r::Preset* preset = m_collection->find_preset(preset_name, false);
        if (preset) {
            std::string new_label = preset->name + suffix(preset);

            if (marker == LABEL_ITEM_PHYSICAL_PRINTER)
                new_label = ph_printer_name + PhysicalPrinter::separator() + new_label;

            if (old_label != new_label)
                SetString(ui_id, WX::from_u8(new_label));
        }
    }
#ifdef __APPLE__
    // wxWidgets on OSX do not upload the text of the combo box line automatically.
    // Force it to update by re-selecting.
    SetSelection(GetSelection());
#endif /* __APPLE __ */
}

}
