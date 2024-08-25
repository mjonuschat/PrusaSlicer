set(SLIC3R_APP_DESKTOP_FILES
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
)

# Create a slic3r-app-desktop executable
# Process mainfests for various platforms.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/slic3r-app-desktop.rc.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-desktop.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../platform/msw/PrusaSlicer.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-desktop.manifest @ONLY)

add_executable(slic3r-app-desktop WIN32 ${SLIC3R_APP_DESKTOP_FILES} 
                                        ${CMAKE_CURRENT_BINARY_DIR}/slic3r-app-desktop.rc)

target_link_libraries(slic3r-app-desktop PUBLIC
        slic3r-base
        slic3r-platform slic3r-platform-wx 
        slic3r-shared slic3r-shared-wx
        ${wxWidgets_LIBRARIES} 
        NanoSVG::nanosvg NanoSVG::nanosvgrast
)

if (MSVC)
    # Manifest is provided through PrusaSlicer.rc, don't generate your own.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
endif ()

slic3r_link_resources(slic3r-app-desktop)
