///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Render/ImguiIconHelper.hpp"

#include <Slic3r/Assert.hpp>

#include <unordered_map>
#include <unordered_set>

namespace Slic3r::App::Render {

static const std::unordered_map<Icon, const char*> ICON_FILENAMES = {
    {Icon::PrintIconMarker, "cog"},
    {Icon::PrintIconMarker, "cog"},
    {Icon::PrinterIconMarker, "printer"},
    {Icon::PrinterSlaIconMarker, "sla_printer"},
    {Icon::FilamentIconMarker, "spool"},
    {Icon::MaterialIconMarker, "resin"},
    {Icon::MinimalizeButton, "notification_minimalize"},
    {Icon::MinimalizeHoverButton, "notification_minimalize_hover"},
    {Icon::RightArrowButton, "notification_right"},
    {Icon::RightArrowHoverButton, "notification_right_hover"},
    {Icon::PreferencesButton, "notification_preferences"},
    {Icon::PreferencesHoverButton, "notification_preferences_hover"},
    {Icon::SliderFloatEditBtnIcon, "edit_button"},
    {Icon::SliderFloatEditBtnPressedIcon, "edit_button_pressed"},
    {Icon::ClipboardBtnIcon, "copy_menu"},
    {Icon::ExpandBtn, "expand_btn"},
    {Icon::CollapseBtn, "collapse_btn"},
    {Icon::RevertButton, "undo"},
    {Icon::WarningMarkerSmall, "notification_warning"},
    {Icon::InfoMarkerSmall, "notification_info"},
    {Icon::PlugMarker, "plug"},
    {Icon::DowelMarker, "dowel"},
    {Icon::SnapMarker, "snap"},
    {Icon::HorizontalHide, "horizontal_hide"},
    {Icon::HorizontalShow, "horizontal_show"},
    {Icon::PrintIdle, "print_idle"},
    {Icon::PrintRunning, "print_running"},
    {Icon::PrintFinished, "print_finished"},
    {Icon::EyeOpen, "dont_print"},
    {Icon::EyeClosed, "dont_print_active"},
    {Icon::SolidPartVolume, "union"},
    {Icon::NegativeVolume, "subtract"},
    {Icon::ModifierVolume, "exclude"},
    {Icon::SupportBlocker, "support_blocker"},
    {Icon::SupportModifier, "support_enforcer"},
    {Icon::TextSolidPartVolume, "add_text_part"},
    {Icon::TextNegativeVolume, "add_text_negative"},
    {Icon::TextModifierVolume, "add_text_modifier"},
    {Icon::SvgSolidPartVolume, "svg_part"},
    {Icon::SvgNegativeVolume, "svg_negative"},
    {Icon::SvgModifierVolume, "svg_modifier"},
    {Icon::ObjectIcon, "object_icon"},
    {Icon::HRModifier, "edit_layers_all"},
    {Icon::CustomSupports, "fdm_supports"},
    {Icon::CustomSeam, "seam_"},
    {Icon::CutConnectors, "cut_connectors"},
    {Icon::MmSegmentation, "mmu_segmentation_"},
    {Icon::Sinking, "sinking"},
    {Icon::FuzzySkin, "fuzzy_skin_painting"},
    {Icon::BedIcon, "bed_object_list"},
    {Icon::Details, "details"},
    {Icon::OpenArrow, "down_arrow"},
    {Icon::CloseArrow, "right_arrow"},
    {Icon::ConfigContainer, "config_container"},
    {Icon::InstancesIcon, "instances_icon"},
    {Icon::SceneMap, "map"},
    {Icon::AddBedIcon, "add_bed"},
    {Icon::OverridesMarker, "overrides_marker"},
    {Icon::AllBeds, "all_beds"},
    {Icon::Lock, "lock_closed"},
    {Icon::LockHovered, "lock_closed_f"},
    {Icon::Unlock, "lock_open"},
    {Icon::UnlockHovered, "lock_open_f"},
    {Icon::DSRevert, "undo_r"},
    {Icon::DSRevertHovered, "undo_f"},
    {Icon::DSRevertDisabled, "undo_disabled"},
    {Icon::DSSettings, "cog_"},
    {Icon::DSSettingsHovered, "cog_f"},
    {Icon::ErrorTick, "error_tick"},
    {Icon::ErrorTickHovered, "error_tick_f"},
    {Icon::PausePrint, "pause_print"},
    {Icon::PausePrintHovered, "pause_print_f"},
    {Icon::EditGCode, "edit_gcode"},
    {Icon::EditGCodeHovered, "edit_gcode_f"},
    {Icon::RemoveTick, "colorchange_del"},
    {Icon::RemoveTickHovered, "colorchange_del_f"},
    // sidebar icons
    {Icon::SavePrint, "save_print"},
    {Icon::SavePrintToFlash, "save_print_to_flash"},
    {Icon::SavePrintToLocal, "save_print_to_local"},
    {Icon::SavePrintAddBookmark, "save_print_add_bookmark"},
    {Icon::LegendTravel, "legend_travel"},
    {Icon::LegendWipe, "legend_wipe"},
    {Icon::LegendRetract, "legend_retract"},
    {Icon::LegendDeretract, "legend_deretract"},
    {Icon::LegendSeams, "legend_seams"},
    {Icon::LegendToolChanges, "legend_toolchanges"},
    {Icon::LegendColorChanges, "legend_colorchanges"},
    {Icon::LegendPausePrints, "legend_pauseprints"},
    {Icon::LegendCustomGCodes, "legend_customgcodes"},
    {Icon::LegendCOG, "legend_cog"},
    {Icon::LegendShells, "legend_shells"},
    {Icon::LegendToolMarker, "legend_toolmarker"},
    {Icon::CloseNotifButton, "notification_close"},
    {Icon::CloseNotifHoverButton, "notification_close_hover"},
    {Icon::EjectButton, "notification_eject_sd"},
    {Icon::EjectHoverButton, "notification_eject_sd_hover"},
    {Icon::WarningMarker, "notification_warning"},
    {Icon::WarningMarkerWhite, "notification_warning_white"},
    {Icon::ErrorMarker, "notification_error"},
    {Icon::CancelButton, "notification_cancel"},
    {Icon::CancelHoverButton, "notification_cancel_hover"},
    {Icon::DocumentationButton, "notification_documentation"},
    {Icon::DocumentationHoverButton, "notification_documentation_hover"},
    {Icon::InfoMarker, "notification_info"},
    {Icon::PlayButton, "notification_play"},
    {Icon::PlayHoverButton, "notification_play_hover"},
    {Icon::PauseButton, "notification_pause"},
    {Icon::PauseHoverButton, "notification_pause_hover"},
    {Icon::OpenButton, "notification_open"},
    {Icon::OpenHoverButton, "notification_open_hover"},
    {Icon::SlaViewOriginal, "sla_view_original"},
    {Icon::SlaViewProcessed, "sla_view_processed"},

    {Icon::MouseLeft, "mouse_left"},
    {Icon::MouseRight, "mouse_right"},
    {Icon::KeyShift, "key_shift"},
    {Icon::KeyEsc, "key_esc"},
    {Icon::KeyDel, "key_del"},

    {Icon::ClippyMarker, "notification_clippy"},
    {Icon::SliceAllBtnIcon, "slice_all"},
    {Icon::WarningMarkerDisabled, "notification_warning_grey"},
    {Icon::PrusaSlicerIcon, "PrusaSlicer"},
    {Icon::CubeViewIcon, "view_cube_test"}, // !tmp, remove after view cube implementation
    // toolbar icons
    {Icon::ToolbarObjects, "toolbar_objects"},
    {Icon::ToolbarAdd, "toolbar_add"},
    {Icon::ToolbarAddInstance, "toolbar_add_instance"},
    {Icon::ToolbarArrange, "toolbar_arrange"},
    {Icon::ToolbarHistory, "toolbar_history"},
    {Icon::ToolbarEllipsis, "toolbar_ellipsis"},
    {Icon::ToolbarGraph, "toolbar_graph"},
    {Icon::ToolbarMove, "toolbar_move"},
    {Icon::ToolbarRotation, "toolbar_rotation"},
    {Icon::ToolbarGCode, "toolbar_gcode"},
    {Icon::ToolbarPaintOnSupports, "toolbar_paint_on_supports"},
    {Icon::ToolbarMeasure, "toolbar_measure"},
    {Icon::ToolbarText, "toolbar_text"},
    {Icon::PrinterNEXT, "printer_NEXT"},
    {Icon::BedThumbnail, "bed_thumbnail"},

    {Icon::SettingsSet, "settings_set"},
    {Icon::TobBarLoad, "tb_load"},
    {Icon::TobBarSave, "tb_save"},
    {Icon::TobBarShowUI, "tb_show_ui"},
    {Icon::TobBarPlus, "plus_new"},
    {Icon::TopBarCross, "cross_new"},

    // Gizmo Paint-On-Supports
    {Icon::Circle, "circle"},
    {Icon::Triangle, "triangle"},
    {Icon::Sphere, "sphere"},
    {Icon::PaintBrush, "paintbrush"},
    {Icon::WandMagicSparkles, "wand-magic-sparkles"},

    {Icon::Calculator, "calculator"},
    {Icon::CopyForGizmo, "copy_for_gizmo"},
    {Icon::NewBtnIcon, "new_button"},
    {Icon::DeleteBtnIcon, "delete_button"},

    // Gizmo Arrange
    {Icon::ArrangeTopLeft, "arrange_top_left"},
    {Icon::ArrangeTopRight, "arrange_top_right"},
    {Icon::ArrangeBottomLeft, "arrange_bottom_left"},
    {Icon::ArrangeBottomRight, "arrange_bottom_right"},
    {Icon::ArrangeCenter, "arrange_center"},

    {Icon::AlignHLeftBtn, "align_horizontal_left"},
    {Icon::AlignHCenterBtn, "align_horizontal_center"},
    {Icon::AlignHRightBtn, "align_horizontal_right"},
    {Icon::AlignVTopBtn, "align_vertical_top"},
    {Icon::AlignVCenterBtn, "align_vertical_center"},
    {Icon::AlignVBottomBtn, "align_vertical_bottom"},

    {Icon::Layers, "layers"},
    {Icon::Infill, "infill"},
    {Icon::SkirtBrim, "skirt_brim"},
    {Icon::Support, "support"},
    {Icon::Time, "time"},
    {Icon::Funnel, "funnel"},
    {Icon::Cog, "cog"},
    {Icon::Cogs, "cogs"},
    {Icon::Output, "output"},
    {Icon::Notes, "notes"},
    {Icon::CaretLeft, "caret_left"},
    {Icon::Search, "search_gray"},
    {Icon::Fan, "cooling"},
    {Icon::AddVolume, "add_volume"},
};

static const std::unordered_set<Icon> ICON_PNG = {
    Icon::BedThumbnail,
    Icon::PrinterNEXT
};

std::string ImguiIconHelper::icon_path(Icon icon)
{
    std::unordered_map<Icon, const char*>::const_iterator it = ICON_FILENAMES.find(icon);
    ASSERT(it != ICON_FILENAMES.cend(), "Icon doesn't have defined filename!", icon);

    std::string filename = it->second;
    if (ICON_PNG.contains(icon)) {
        filename.append(".png");
    } else {
        filename.append(".svg");
    }

    return "icons/" + filename;
}

std::string ImguiIconHelper::icon_name(Icon icon)
{
    std::unordered_map<Icon, const char*>::const_iterator it = ICON_FILENAMES.find(icon);
    ASSERT(it != ICON_FILENAMES.cend(), "Icon doesn't have defined filename!", icon);

    return it->second;
}

}
