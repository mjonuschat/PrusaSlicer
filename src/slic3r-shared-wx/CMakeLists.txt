set(SLIC3R_SHARED_WX_FILES
        include/Slic3r/App/WX/format.hpp
        include/Slic3r/App/WX/BitmapCache.hpp
        src/Slic3r/App/WX/BitmapCache.cpp
        include/Slic3r/App/WX/wxExtensions.hpp
        src/Slic3r/App/WX/wxExtensions.cpp
        include/Slic3r/App/WX/WidgetsConfig.hpp
        src/Slic3r/App/WX/WidgetsConfig.cpp
        include/Slic3r/App/WX/StringConversions.hpp
        src/Slic3r/App/WX/StringConversions.cpp
        include/Slic3r/App/WX/MenuManaging.hpp
        src/Slic3r/App/WX/MenuManaging.cpp
        include/Slic3r/App/WX/BitmapGetters.hpp
        src/Slic3r/App/WX/BitmapGetters.cpp
        include/Slic3r/App/WX/Scalable.hpp
        src/Slic3r/App/WX/Scalable.cpp
        include/Slic3r/App/WX/ExtraRenderers.hpp
        src/Slic3r/App/WX/ExtraRenderers.cpp
        include/Slic3r/App/WX/Highlighter.hpp
        src/Slic3r/App/WX/Highlighter.cpp
        include/Slic3r/App/WX/LockButton.hpp
        src/Slic3r/App/WX/LockButton.cpp
        include/Slic3r/App/WX/MacUtils.hpp
        include/Slic3r/App/WX/MsgDialog.hpp
        src/Slic3r/App/WX/MsgDialog.cpp
        include/Slic3r/App/WX/ConfigManipulation.hpp
        src/Slic3r/App/WX/ConfigManipulation.cpp
        include/Slic3r/App/WX/I18N.hpp
        src/Slic3r/App/WX/I18N.cpp
        # our widgets
        include/Slic3r/App/WX/Widgets/UIColors.hpp
        include/Slic3r/App/WX/Widgets/StateColor.hpp
        src/Slic3r/App/WX/Widgets/StateColor.cpp
        include/Slic3r/App/WX/Widgets/StateHandler.hpp
        src/Slic3r/App/WX/Widgets/StateHandler.cpp
        include/Slic3r/App/WX/Widgets/StaticBox.hpp
        src/Slic3r/App/WX/Widgets/StaticBox.cpp
        include/Slic3r/App/WX/Widgets/TextInput.hpp
        src/Slic3r/App/WX/Widgets/TextInput.cpp
        include/Slic3r/App/WX/Widgets/SpinInput.hpp
        src/Slic3r/App/WX/Widgets/SpinInput.cpp
        include/Slic3r/App/WX/Widgets/DropDown.hpp
        src/Slic3r/App/WX/Widgets/DropDown.cpp
        include/Slic3r/App/WX/Widgets/CheckBox.hpp
        src/Slic3r/App/WX/Widgets/CheckBox.cpp
        include/Slic3r/App/WX/Widgets/ComboBox.hpp
        src/Slic3r/App/WX/Widgets/ComboBox.cpp
        include/Slic3r/App/WX/Widgets/Button.hpp
        src/Slic3r/App/WX/Widgets/Button.cpp
        include/Slic3r/App/WX/Widgets/BitmapToggleButton.hpp
        src/Slic3r/App/WX/Widgets/BitmapToggleButton.cpp
        include/Slic3r/App/WX/Widgets/BitmapComboBox.hpp
        src/Slic3r/App/WX/Widgets/BitmapComboBox.cpp
        include/Slic3r/App/WX/Widgets/SwitchButton.hpp
        src/Slic3r/App/WX/Widgets/SwitchButton.cpp
)

if (APPLE)
    set(SLIC3R_SHARED_WX_FILES 
        ${SLIC3R_SHARED_WX_FILES}
        src/Slic3r/App/WX/MacUtils.mm
    )
endif()

add_library(slic3r-shared-wx STATIC ${SLIC3R_SHARED_WX_FILES})
target_include_directories(slic3r-shared-wx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-shared-wx PUBLIC
        slic3r-shared
        libslic3r # For Utils.hpp
        Boost::boost
        localesutils
)

if (UNIX AND NOT APPLE)
    find_package(GTK${SLIC3R_GTK} REQUIRED)
    target_include_directories(slic3r-shared-wx PRIVATE ${GTK${SLIC3R_GTK}_INCLUDE_DIRS})
    target_link_libraries(slic3r-shared-wx PUBLIC ${GTK${SLIC3R_GTK}_LIBRARIES} fontconfig)
endif ()


# Doxygen
add_doxygen_target(slic3r-shared-wx)
