///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <wx/bmpbndl.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include "Slic3r/App/WX/Widgets/BitmapComboBox.hpp"
#include "libslic3r/Preset.hpp"

namespace Slic3r::App::WX {
class BitmapCache;
}

namespace Slic3r::App::Desktop::Preset {

// BitmapComboBox used to presets list on Sidebar and Tabs
class PresetComboBox : public WX::Widgets::BitmapComboBox
{
public:
    PresetComboBox(wxWindow* parent, Slic3r::Preset::Type preset_type, const wxSize& size = wxDefaultSize, PresetBundle* preset_bundle = nullptr);
    ~PresetComboBox();

    void    init_from_bundle(PresetBundle* preset_bundle);
    bool    set_printer_technology(PrinterTechnology pt);
    void    show_all(bool show_all);
    bool    is_selected_physical_printer();
    // select preset which is selected in PreseBundle
    void    update_from_bundle();

    void    set_selection_changed_function(std::function<void(int)> sel_changed) { on_selection_changed = sel_changed; }
    void    show_modif_preset_separately() { m_show_modif_preset_separately = true; }

    virtual void msw_rescale();
    virtual void sys_color_changed();
    virtual void OnSelect(wxCommandEvent& evt);

    // used by Filaments list to update preset list according to the particular extruder
    void    set_extruder_idx(int extruder_idx) { m_extruder_idx = extruder_idx; }//?
    int     get_extruder_idx()                 { return m_extruder_idx; }//?
    // Return true, if physical printer was selected 
    // and next internal selection was accomplished
    bool    selection_is_changed_according_to_physical_printers();//?

protected: 

	enum LabelItemType {
		LABEL_ITEM_PHYSICAL_PRINTER = 0xffffff01,
		LABEL_ITEM_DISABLED,
		LABEL_ITEM_MARKER,
		LABEL_ITEM_PHYSICAL_PRINTERS,
		LABEL_ITEM_WIZARD_PRINTERS,
        LABEL_ITEM_WIZARD_FILAMENTS,
        LABEL_ITEM_WIZARD_MATERIALS,

        LABEL_ITEM_MAX,
	};

    void set_label_marker(int item, LabelItemType label_item_type = LABEL_ITEM_MARKER);

#ifdef __linux__
    static const char*      separator_head() { return "------- "; }
    static const char*      separator_tail() { return " -------"; }
#else // __linux__ 
    static const char*      separator_head() { return "————— "; }
    static const char*      separator_tail() { return " —————"; }
#endif // __linux__
    static wxString         separator(const std::string& label);
    std::string             suffix(const Slic3r::Preset& preset);
    std::string             suffix(Slic3r::Preset* preset);

    Slic3r::Preset::Type    get_type() { return m_type; }

    virtual wxString        get_preset_name(const Slic3r::Preset& preset); 
    virtual void            update();

    void    update(std::string select_preset);
    void    update_selection();
    void    invalidate_selection();
    void    validate_selection(bool predicate = false);
    bool    allow_templates() const;

    wxBitmapBundle*     get_bmp(  std::string bitmap_key, bool wide_icons, const std::string& main_icon_name,
                        bool is_compatible = true, bool is_system = false, bool is_single_bar = false,
                        const std::string& filament_rgb = "", const std::string& extruder_rgb = "", const std::string& material_rgb = "");

    wxBitmapBundle*     get_bmp(  std::string bitmap_key, const std::string& main_icon_name, const std::string& next_icon_name,
                        bool is_enabled = true, bool is_compatible = true, bool is_system = false);

    wxBitmapBundle          NullBitmapBndl();
    static WX::BitmapCache& bitmap_cache();

private:
    void fill_width_height();

protected:
    typedef std::size_t Marker;

    struct PresetData {
        wxString        name;
        wxString        lower_name;
        wxBitmapBundle* bitmap;
        bool            enabled; // not used in incomp_presets
    };

    std::function<void(int)>    on_selection_changed { nullptr };

    Slic3r::Preset::Type    m_type;
    std::string             m_main_bitmap_name;

    PresetBundle*           m_preset_bundle         {nullptr};
    PresetCollection*       m_collection            {nullptr};

    // Caching bitmaps for the all bitmaps, used in preset comboboxes

    int     m_last_selected;
    int     m_em_unit;
    bool    m_suppress_change   { true };

    // This parameter is used by FilamentSettings tab to show filament setting related to the active extruder
    int     m_extruder_idx      { 0 };//?

private:
    bool    m_show_all{ false };
    bool    m_show_modif_preset_separately{ false };

    // parameters for an icon's drawing
    int     m_icon_height;
    int     m_norm_icon_width;
    int     m_null_icon_width;
    int     m_thin_icon_width;
    int     m_wide_icon_width;
    int     m_space_icon_width;
    int     m_thin_space_icon_width;
    int     m_wide_space_icon_width;

    PrinterTechnology   m_printer_technology    {ptAny};
    // Indicator, that the preset is compatible with the selected printer.
    wxBitmapBundle*     m_bitmapCompatible  {nullptr};
    // Indicator, that the preset is NOT compatible with the selected printer.
    wxBitmapBundle*     m_bitmapIncompatible {nullptr};

};

} 
