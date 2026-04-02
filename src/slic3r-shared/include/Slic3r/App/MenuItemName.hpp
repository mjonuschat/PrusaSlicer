#pragma once

namespace Slic3r::App {

enum class MenuItemName
{
    Separator,
    MainMenu,

    Edit,
    SelectAll,
    DeselectAll,
    DeleteSelected,
    Undo,
    Redo,
    Copy,
    Paste,
    ReloadFromDisk,
    Search,

    ShapeGallery,

    View,
    ShowLabel,
    FullScreen,
    ZoomIn,
    ZoomOut,
    CameraProjectionSwitch,
    LookAtActiveBed,
    CameraDefaultView,
    CameraTopView,
    CameraBottomView,
    CameraFrontView,
    CameraRearView,
    CameraLeftView,
    CameraRightView,

    Preferences,

    Configuration,
    ConfigurationWizard,
    CheckConfigUpdates,
    CheckAppUpdates,
    FlashPrinterFirmware,

    Help,
    PSWebsite,
    QuickStart,
    Samples,
    Prusa3DDrivers,
    Releases,
    SystemInfo,
    ConfigFolder,
    ReportAnIssue,
    About,
    TipOfTheDay,
    KeyboardShortcutsDialog,

    Exit,

    FileMenu,
    NewProject,
    OpenProject,
    SaveProject,
    SaveProjectAs,

    RecentProjects,

    Import,
    ImportGeometry,

    Export,
    ExportGcode,
    SendGcode,
    ExportGcodeToFlash,

    OnlinePresetUpdate,

    JumpToValue,

    BedContextMenu,
    AddObjectShape,
    ObjectShapeLoad,
    ObjectShapeCube,
    ObjectShapeCylinder,
    ObjectShapeSphere,
    ObjectShapeText,
    DeleteBed,

    ObjectMenu,
    AddVolume,
    SolidPartVolume,
    NegativeVolume,
    ModifierVolume,
    SupportBlocker,
    SupportModifier,
    SolidPartVolumeShapeLoad,
    SolidPartVolumeShapeCube,
    SolidPartVolumeShapeCylinder,
    SolidPartVolumeShapeSphere,
    NegativeVolumeShapeLoad,
    NegativeVolumeShapeCube,
    NegativeVolumeShapeCylinder,
    NegativeVolumeShapeSphere,
    ModifierVolumeShapeLoad,
    ModifierVolumeShapeCube,
    ModifierVolumeShapeCylinder,
    ModifierVolumeShapeSphere,
    SupportBlockerShapeLoad,
    SupportBlockerShapeCube,
    SupportBlockerShapeCylinder,
    SupportBlockerShapeSphere,
    SupportModifierShapeLoad,
    SupportModifierShapeCube,
    SupportModifierShapeCylinder,
    SupportModifierShapeSphere,
    DeleteSelectedObject,
};
} // namespace Slic3r::App
