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
    ChangeCameraType,

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
    KeyboardShortcuts,

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

    JumpToValue,
};
} // namespace Slic3r::App
