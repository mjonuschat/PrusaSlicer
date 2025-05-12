set(SLIC3R_APP_DESKTOP_FILES
	include/Slic3r/App/Desktop/Run.hpp

    src/Slic3r/App/Desktop/DesktopApp.cpp
    src/Slic3r/App/Desktop/DesktopApp.hpp
    src/Slic3r/App/Desktop/MainFrame.cpp
    src/Slic3r/App/Desktop/MainFrame.hpp
    src/Slic3r/App/Desktop/TopBar.cpp
    src/Slic3r/App/Desktop/TopBar.hpp
    src/Slic3r/App/Desktop/TopBarMenus.cpp
    src/Slic3r/App/Desktop/TopBarMenus.hpp
    src/Slic3r/App/Desktop/ModePaletteComboBox.cpp
    src/Slic3r/App/Desktop/ModePaletteComboBox.hpp
    src/Slic3r/App/Desktop/GUI_Descriptions.cpp
    src/Slic3r/App/Desktop/GUI_Descriptions.hpp
    src/Slic3r/App/Desktop/SingleInstanceCheckerFactory.hpp
    src/Slic3r/App/Desktop/AppInstanceCheck.hpp
    src/Slic3r/App/Desktop/AppInstanceCheck.cpp
    src/Slic3r/App/Desktop/SecretStoreFactory.hpp

    src/Slic3r/App/Desktop/Config/Field.cpp
    src/Slic3r/App/Desktop/Config/Field.hpp
    src/Slic3r/App/Desktop/Config/OptionsGroup.cpp
    src/Slic3r/App/Desktop/Config/OptionsGroup.hpp
    src/Slic3r/App/Desktop/Config/OG_CustomCtrl.cpp
    src/Slic3r/App/Desktop/Config/OG_CustomCtrl.hpp

    src/Slic3r/App/Desktop/Preset/SubstitutionManager.cpp
    src/Slic3r/App/Desktop/Preset/SubstitutionManager.hpp
    src/Slic3r/App/Desktop/Preset/EditorPage.cpp
    src/Slic3r/App/Desktop/Preset/EditorPage.hpp
    src/Slic3r/App/Desktop/Preset/AbstractEditor.cpp
    src/Slic3r/App/Desktop/Preset/AbstractEditor.hpp
    src/Slic3r/App/Desktop/Preset/EditorPrint.cpp
    src/Slic3r/App/Desktop/Preset/EditorPrint.hpp
    src/Slic3r/App/Desktop/Preset/EditorFilament.cpp
    src/Slic3r/App/Desktop/Preset/EditorFilament.hpp
    src/Slic3r/App/Desktop/Preset/EditorSLAPrint.cpp
    src/Slic3r/App/Desktop/Preset/EditorSLAPrint.hpp
    src/Slic3r/App/Desktop/Preset/EditorSLAMaterial.cpp
    src/Slic3r/App/Desktop/Preset/EditorSLAMaterial.hpp
    src/Slic3r/App/Desktop/Preset/EditorPrinter.cpp
    src/Slic3r/App/Desktop/Preset/EditorPrinter.hpp
    src/Slic3r/App/Desktop/Preset/PresetNameGetter.cpp
    src/Slic3r/App/Desktop/Preset/PresetNameGetter.hpp
    src/Slic3r/App/Desktop/Preset/SavePresetDialog.cpp
    src/Slic3r/App/Desktop/Preset/SavePresetDialog.hpp
    src/Slic3r/App/Desktop/Preset/EditGCodeDialog.cpp
    src/Slic3r/App/Desktop/Preset/EditGCodeDialog.hpp
    src/Slic3r/App/Desktop/Preset/PresetComboBox.cpp
    src/Slic3r/App/Desktop/Preset/PresetComboBox.hpp
    src/Slic3r/App/Desktop/Preset/EditorPresetComboBox.cpp
    src/Slic3r/App/Desktop/Preset/EditorPresetComboBox.hpp
    src/Slic3r/App/Desktop/Preset/Manipulators.cpp
    src/Slic3r/App/Desktop/Preset/Manipulators.hpp

	
)

if(WIN32)
    list(APPEND SLIC3R_APP_DESKTOP_FILES
		src/Slic3r/App/Desktop/SingleInstanceCheckerFactoryWin32.cpp
        src/Slic3r/App/Desktop/SecretStoreFactoryWX.cpp
    )
elseif(APPLE)
     list(APPEND SLIC3R_APP_DESKTOP_FILES
        src/Slic3r/App/Desktop/SingleInstanceCheckerFactoryLock.cpp
        src/Slic3r/App/Desktop/SecretStoreFactoryWX.cpp
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
     list(APPEND SLIC3R_APP_DESKTOP_FILES
        src/Slic3r/App/Desktop/SingleInstanceCheckerFactoryLock.cpp
        src/Slic3r/App/Desktop/SecretStoreFactoryLinux.cpp
    )
endif()

add_library(slic3r-app-desktop STATIC ${SLIC3R_APP_DESKTOP_FILES})

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT slic3r-app-desktop)

find_package(NanoSVG REQUIRED)

target_link_libraries(slic3r-app-desktop PUBLIC
    slic3r-shared-wx
)

target_include_directories(slic3r-app-desktop PUBLIC include)

# See https://github.com/prusa3d/PrusaSlicer/issues/4584.
if (UNIX AND NOT APPLE)
    find_package(GTK3 REQUIRED)
    target_link_libraries(slic3r-app-desktop PUBLIC GTK3::GTK3)
endif ()
