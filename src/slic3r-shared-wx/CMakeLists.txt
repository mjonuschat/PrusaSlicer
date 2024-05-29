set(SLIC3R_SHARED_WX_FILES
        src/Slic3r/App/WX/BitmapCache.hpp
        src/Slic3r/App/WX/BitmapCache.cpp
        src/Slic3r/App/WX/wxExtensions.hpp
        src/Slic3r/App/WX/wxExtensions.cpp
        src/Slic3r/App/WX/WidgetsConfig.hpp
        src/Slic3r/App/WX/WidgetsConfig.cpp
        src/Slic3r/App/WX/StringConversions.hpp
        src/Slic3r/App/WX/StringConversions.cpp
        src/Slic3r/App/WX/MenuManaging.hpp
        src/Slic3r/App/WX/MenuManaging.cpp
        src/Slic3r/App/WX/BitmapGetters.hpp
        src/Slic3r/App/WX/BitmapGetters.cpp
        src/Slic3r/App/WX/Scalable.hpp
        src/Slic3r/App/WX/Scalable.cpp
        src/Slic3r/App/WX/Highlighter.hpp
        src/Slic3r/App/WX/Highlighter.cpp
        src/Slic3r/App/WX/LockButton.hpp
        src/Slic3r/App/WX/LockButton.cpp
        # our widgets
        src/Slic3r/App/WX/Widgets/UIColors.hpp
        src/Slic3r/App/WX/Widgets/StateColor.hpp
        src/Slic3r/App/WX/Widgets/StateColor.cpp
        src/Slic3r/App/WX/Widgets/StateHandler.hpp
        src/Slic3r/App/WX/Widgets/StateHandler.cpp
        src/Slic3r/App/WX/Widgets/StaticBox.hpp
        src/Slic3r/App/WX/Widgets/StaticBox.cpp
        src/Slic3r/App/WX/Widgets/TextInput.hpp
        src/Slic3r/App/WX/Widgets/TextInput.cpp
        src/Slic3r/App/WX/Widgets/SpinInput.hpp
        src/Slic3r/App/WX/Widgets/SpinInput.cpp
        src/Slic3r/App/WX/Widgets/DropDown.hpp
        src/Slic3r/App/WX/Widgets/DropDown.cpp
        src/Slic3r/App/WX/Widgets/CheckBox.hpp
        src/Slic3r/App/WX/Widgets/CheckBox.cpp
        src/Slic3r/App/WX/Widgets/ComboBox.hpp
        src/Slic3r/App/WX/Widgets/ComboBox.cpp
        src/Slic3r/App/WX/Widgets/Button.hpp
        src/Slic3r/App/WX/Widgets/Button.cpp
        src/Slic3r/App/WX/Widgets/BitmapToggleButton.hpp
        src/Slic3r/App/WX/Widgets/BitmapToggleButton.cpp
        src/Slic3r/App/WX/Widgets/SwitchButton.hpp
        src/Slic3r/App/WX/Widgets/SwitchButton.cpp
)

add_library(slic3r-shared-wx STATIC ${SLIC3R_SHARED_WX_FILES})
target_include_directories(slic3r-shared-wx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-shared-wx PUBLIC Boost::boost)
