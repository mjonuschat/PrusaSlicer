#include "Slic3r/App/MenuBuilder.hpp"
#include "Slic3r/App/MenuManager.hpp"
#include "Slic3r/App/MenuItem.hpp"
#include "Slic3r/App/CommandBindingManager.hpp"
#include "Slic3r/App/UIItemCommand.hpp"

#include "Slic3r/App/Yoga/Menu.hpp"
#include "Slic3r/App/Yoga/MenuItem.hpp"

#include "Slic3r/App/Render/ImguiIconHelper.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

std::string dots{"..."};

std::string MenuBuilder::item_name_translated(MenuItemName menu_item_name)
{
    switch (menu_item_name) {
    case MenuItemName::MainMenu:
        return Biz::_u8L("Menu");
    case MenuItemName::Edit:
        return Biz::_u8L("Edit");
    case MenuItemName::SelectAll:
        return Biz::_u8L("Select All");
    case MenuItemName::DeselectAll:
        return Biz::_u8L("Deselect All");
    case MenuItemName::DeleteSelected:
        return Biz::_u8L("Delete selected");
    case MenuItemName::Undo:
        return Biz::_u8L("Undo");
    case MenuItemName::Redo:
        return Biz::_u8L("Redo");
    case MenuItemName::Copy:
        return Biz::_u8L("Copy");
    case MenuItemName::Paste:
        return Biz::_u8L("Paste");
    case MenuItemName::ReloadFromDisk:
        return Biz::_u8L("Reload From Disk");
    case MenuItemName::Search:
        return Biz::_u8L("Search");
    case MenuItemName::Preferences:
#ifdef __APPLE__
        return Biz::_u8L("Settings");
#else
        return Biz::_u8L("Preferences");
#endif
    case MenuItemName::FileMenu:
        return Biz::_u8L("File");
    case MenuItemName::NewProject:
        return Biz::_u8L("New project");
    case MenuItemName::OpenProject:
        return Biz::_u8L("Open project");
    case MenuItemName::SaveProject:
        return Biz::_u8L("Save");
    case MenuItemName::SaveProjectAs:
        return Biz::_u8L("Save as") + dots;
    case MenuItemName::Import:
        return Biz::_u8L("Import");
    case MenuItemName::ImportGeometry:
        return Biz::_u8L("Import STL/3MF");
    case MenuItemName::JumpToValue:
        return Biz::_u8L("Jump to height");
    case MenuItemName::ShapeGallery:
        return Biz::_u8L("Shape Gallery");
    case MenuItemName::View:
        return Biz::_u8L("View");
    case MenuItemName::ShowLabel:
        return Biz::_u8L("Show Labels");
    case MenuItemName::FullScreen:
        return Biz::_u8L("Fullscreen");
    case MenuItemName::ZoomIn:
        return Biz::_u8L("Zoom In");
    case MenuItemName::ZoomOut:
        return Biz::_u8L("Zoom Out");
    case MenuItemName::CameraProjectionSwitch:
        return Biz::_u8L("Switch Projection");
    case MenuItemName::LookAtActiveBed:
        return Biz::_u8L("Look at Active Bed");
    case MenuItemName::CameraDefaultView:
        return Biz::_u8L("Default View");
    case MenuItemName::CameraTopView:
        return Biz::_u8L("Top View");
    case MenuItemName::CameraBottomView:
        return Biz::_u8L("Bottom View");
    case MenuItemName::CameraFrontView:
        return Biz::_u8L("Front View");
    case MenuItemName::CameraRearView:
        return Biz::_u8L("Rear View");
    case MenuItemName::CameraLeftView:
        return Biz::_u8L("Left View");
    case MenuItemName::CameraRightView:
        return Biz::_u8L("Right View");
    case MenuItemName::Configuration:
        return Biz::_u8L("Configuration");
    case MenuItemName::ConfigurationWizard:
        return Biz::_u8L("Configuration Wizard");
    case MenuItemName::CheckConfigUpdates:
        return Biz::_u8L("Check for Config Updates");
    case MenuItemName::CheckAppUpdates:
        return Biz::_u8L("Check for Application Updates");
    case MenuItemName::FlashPrinterFirmware:
        return Biz::_u8L("Flash Printer Firmware");
    case MenuItemName::Help:
        return Biz::_u8L("Help");
    case MenuItemName::PSWebsite:
        return Biz::_u8L("PrusaSlicer Website");
    case MenuItemName::QuickStart:
        return Biz::_u8L("Quick Start");
    case MenuItemName::Samples:
        return Biz::_u8L("Sample G-codes and Models");
    case MenuItemName::Prusa3DDrivers:
        return Biz::_u8L("Prusa 3D Drivers");
    case MenuItemName::Releases:
        return Biz::_u8L("Software Releases");
    case MenuItemName::SystemInfo:
        return Biz::_u8L("System Info");
    case MenuItemName::ConfigFolder:
        return Biz::_u8L("Show Configuration Folder");
    case MenuItemName::ReportAnIssue:
        return Biz::_u8L("Report an Issue");
    case MenuItemName::About:
        return Biz::_u8L("About PrusaSlicer");
    case MenuItemName::TipOfTheDay:
        return Biz::_u8L("Show Tip of the Day");
    case MenuItemName::KeyboardShortcutsDialog:
        return Biz::_u8L("Keyboard Shortcuts");
    case MenuItemName::RecentProjects:
        return Biz::_u8L("Recent Projects");
    case MenuItemName::Exit:
        return Biz::_u8L("Exit");
    case MenuItemName::Export:
        return Biz::_u8L("Export");
    case MenuItemName::ExportGcode:
        return Biz::_u8L("Export G-code");
    case MenuItemName::SendGcode:
        return Biz::_u8L("Send G-code");
    case MenuItemName::ExportGcodeToFlash:
        return Biz::_u8L("Export G-code to SD Card / Flash Drive");
    case MenuItemName::OnlinePresetUpdate:
        return Biz::_u8L("Update from Online Presets");
    case MenuItemName::DeleteBed:
    case MenuItemName::DeleteSelectedObject:
        return Biz::_u8L("Delete");
    case MenuItemName::AddObjectShape:
        return Biz::_u8L("Add shape");
    case MenuItemName::ObjectShapeLoad:
    case MenuItemName::SolidPartVolumeShapeLoad:
    case MenuItemName::NegativeVolumeShapeLoad:
    case MenuItemName::ModifierVolumeShapeLoad:
    case MenuItemName::SupportBlockerShapeLoad:
    case MenuItemName::SupportModifierShapeLoad:
        return Biz::_u8L("Load");
    case MenuItemName::ObjectShapeCube:
    case MenuItemName::SolidPartVolumeShapeCube:
    case MenuItemName::NegativeVolumeShapeCube:
    case MenuItemName::ModifierVolumeShapeCube:
    case MenuItemName::SupportBlockerShapeCube:
    case MenuItemName::SupportModifierShapeCube:
        return Biz::_u8L("Cube");
    case MenuItemName::ObjectShapeCylinder:
    case MenuItemName::SolidPartVolumeShapeCylinder:
    case MenuItemName::NegativeVolumeShapeCylinder:
    case MenuItemName::ModifierVolumeShapeCylinder:
    case MenuItemName::SupportBlockerShapeCylinder:
    case MenuItemName::SupportModifierShapeCylinder:
        return Biz::_u8L("Cylinder");
    case MenuItemName::ObjectShapeSphere:
    case MenuItemName::SolidPartVolumeShapeSphere:
    case MenuItemName::NegativeVolumeShapeSphere:
    case MenuItemName::ModifierVolumeShapeSphere:
    case MenuItemName::SupportBlockerShapeSphere:
    case MenuItemName::SupportModifierShapeSphere:
        return Biz::_u8L("Sphere");
    case MenuItemName::ObjectShapeText:
        return Biz::_u8L("Text");
    case MenuItemName::AddVolume:
        return Biz::_u8L("Add volume");
    case MenuItemName::SolidPartVolume:
        return Biz::_u8L("Solid part");
    case MenuItemName::NegativeVolume:
        return Biz::_u8L("Negative volume");
    case MenuItemName::ModifierVolume:
        return Biz::_u8L("Modifier");
    case MenuItemName::SupportBlocker:
        return Biz::_u8L("Support blocker");
    case MenuItemName::SupportModifier:
        return Biz::_u8L("Support modifier");

    default:
        return std::string();
    }
};

Render::Icon MenuBuilder::item_icon(MenuItemName menu_item_name)
{
    switch (menu_item_name) {
    case MenuItemName::MainMenu:
        return Render::Icon::PrusaSlicerIcon;
    case MenuItemName::DeleteSelected:
    case MenuItemName::DeleteBed:
    case MenuItemName::DeleteSelectedObject:
        return Render::Icon::DeleteBtnIcon;
    case MenuItemName::Search:
        return Render::Icon::Search;
    case MenuItemName::Preferences:
        return Render::Icon::Cog;
    case MenuItemName::FileMenu:
    case MenuItemName::NewProject:
        return Render::Icon::NewBtnIcon;
    case MenuItemName::OpenProject:
        return Render::Icon::OpenFolder;
    case MenuItemName::SaveProject:
    case MenuItemName::SaveProjectAs:
        return Render::Icon::TobBarSave;
    case MenuItemName::ImportGeometry:
        return Render::Icon::AddObject;
    case MenuItemName::ShapeGallery:
    case MenuItemName::AddObjectShape:
        return Render::Icon::Shapes;
    case MenuItemName::RecentProjects:
        return Render::Icon::RecentProjects;
    case MenuItemName::AddVolume:
        return Render::Icon::AddVolume;
    case MenuItemName::SolidPartVolume:
        return Render::Icon::SolidPartVolume;
    case MenuItemName::NegativeVolume:
        return Render::Icon::NegativeVolume;
    case MenuItemName::ModifierVolume:
        return Render::Icon::ModifierVolume;
    case MenuItemName::SupportBlocker:
        return Render::Icon::SupportBlocker;
    case MenuItemName::SupportModifier:
        return Render::Icon::SupportModifier;

    case MenuItemName::Edit:
    case MenuItemName::Import:
    case MenuItemName::DeselectAll:
    case MenuItemName::JumpToValue:
    case MenuItemName::BedContextMenu:
    case MenuItemName::SolidPartVolumeShapeLoad:
    default:
        return Render::Icon::None;
    }
}

std::string MenuBuilder::icon_name(MenuItemName menu_item_name)
{
    if (Render::Icon icon = item_icon(menu_item_name); icon == Render::Icon::None) {
        return std::string();
    } else {
        return Render::ImguiIconHelper::icon_name(icon);
    }
    return std::string();
}

void MenuBuilder::add_submenu(Yoga::MenuItem* yoga_menu_item, App::MenuItem* menu_item)
{
    for (App::MenuItem* sub_menu_item : menu_item->children()) {
        if (sub_menu_item->is_separator()) {
            yoga_menu_item->append_sub_menu_separator();
            continue;
        }
        Yoga::MenuItem* new_yoga_menu_item = yoga_menu_item->append_sub_menu_item(
            item_name_translated(sub_menu_item->name()),
            nullptr,
            item_icon(sub_menu_item->name())
        );
        if (sub_menu_item->children().empty()) {
            m_command_binding_manager.bind_menu_item(sub_menu_item->command(), new_yoga_menu_item);
        } else {
            add_submenu(new_yoga_menu_item, sub_menu_item);
        }
    }
}

Yoga::MenuItem* MenuBuilder::add_menu_item(Yoga::Menu* menu, App::MenuItem* menu_item)
{
    Yoga::MenuItem* yoga_menu_item = menu->append_item(
        item_name_translated(menu_item->name()),
        nullptr,
        item_icon(menu_item->name())
    );
    m_command_binding_manager.bind_menu_item(menu_item->command(), yoga_menu_item);

    return yoga_menu_item;
}

void MenuBuilder::add_menu_items(Yoga::Menu* menu, App::MenuItem* root_menu_item)
{
    ASSERT(root_menu_item);

    for (App::MenuItem* menu_item : root_menu_item->children()) {
        if (menu_item->is_separator()) {
            menu->append_separator();
        } else if (menu_item->children().empty()) {
            Yoga::MenuItem* yoga_menu_item = add_menu_item(menu, menu_item);
        } else {
            Yoga::MenuItem* yoga_menu_item_with_submenu =
                menu->append_item_as_menu(item_name_translated(menu_item->name()));
            add_submenu(yoga_menu_item_with_submenu, menu_item);
        }
    }
}

Yoga::MenuItem* MenuBuilder::add_menu_item(Yoga::Menu* menu, MenuItemName menu_item_name)
{
    return add_menu_item(menu, m_menu_manager.menu_item(menu_item_name));
}
} // namespace Slic3r::App
