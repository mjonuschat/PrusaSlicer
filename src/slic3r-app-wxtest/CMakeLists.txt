set(SLIC3R_APP_WXTEST_FILES
        src/Slic3r/App/WXTest/TestApp.cpp
        src/Slic3r/App/WXTest/TestApp.hpp
        src/Slic3r/App/WXTest/MainFrame.cpp
        src/Slic3r/App/WXTest/MainFrame.hpp
)

if (WIN32)
    add_executable(slic3r-app-wxtest WIN32 ${SLIC3R_APP_WXTEST_FILES})
else()
    add_executable(slic3r-app-wxtest ${SLIC3R_APP_WXTEST_FILES})
endif()

target_link_libraries(slic3r-app-wxtest PUBLIC
        slic3r-platform slic3r-platform-wx slic3r-shared
        ${wxWidgets_LIBRARIES} NanoSVG::nanosvg NanoSVG::nanosvgrast
)


if (WIN32)
    if (CMAKE_CONFIGURATION_TYPES)
        foreach (CONF ${CMAKE_CONFIGURATION_TYPES})
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}" WIN_CONF_OUTPUT_DIR)
            file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${CONF}/resources" WIN_RESOURCES_SYMLINK)
            add_custom_command(TARGET slic3r-app-wxtest POST_BUILD
                    COMMAND if exist "${WIN_CONF_OUTPUT_DIR}" "("
                    if not exist "${WIN_RESOURCES_SYMLINK}" "("
                    mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_NEXT_DIR_WIN}"
                    ")"
                    ")"
                    COMMENT "Symlinking the resources directory into the build tree"
                    VERBATIM
            )

            #            if (SLIC3R_GUI)
            #                add_custom_command(TARGET slic3r-app-wxtest POST_BUILD
            #                        COMMAND ${CMAKE_COMMAND} -E copy
            #                        "${wxWidgets_ROOT_DIR}/bin/WebView2Loader.dll"
            #                        "${CMAKE_CURRENT_BINARY_DIR}/${CONF}/WebView2Loader.dll")
            #            endif()
        endforeach ()
    else ()
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/resources" WIN_RESOURCES_SYMLINK)
        add_custom_command(TARGET slic3r-app-wxtest POST_BUILD
                COMMAND if not exist "${WIN_RESOURCES_SYMLINK}" "(" mklink /J "${WIN_RESOURCES_SYMLINK}" "${SLIC3R_RESOURCES_NEXT_DIR_WIN}" ")"
                COMMENT "Symlinking the resources directory into the build tree"
                VERBATIM
        )
        #        if (SLIC3R_GUI)
        #            add_custom_command(TARGET slic3r-app-wxtest POST_BUILD
        #                    COMMAND ${CMAKE_COMMAND} -E copy
        #                    "${wxWidgets_ROOT_DIR}/bin/WebView2Loader.dll"
        #                    "${CMAKE_CURRENT_BINARY_DIR}/${CONF}/WebView2Loader.dll")
        #        endif()
    endif ()

    # This has to be a separate target due to the windows command line lenght limits
    #    add_custom_target(slic3r-app-wxtestDllsCopy ALL DEPENDS ShaderManager)
    #    slic3r-app-wxtest_copy_dlls(ShaderManagerDllsCopy)
elseif(EMSCRIPTEN)
    find_package(Boost REQUIRED COMPONENTS filesystem)
    target_link_libraries(slic3r-app-wxtest PUBLIC Boost::filesystem)
    message(STATUS "Preload file Flags: " --preload-file "${SLIC3R_RESOURCES_NEXT_DIR}@/resources")

    set(CMAKE_EXE_LINKER_FLAGS "--preload-file ${SLIC3R_RESOURCES_NEXT_DIR}@/resources -pthread ${CMAKE_EXE_LINKER_FLAGS}")
else ()
    #    if (APPLE)
    #        # On OSX, the name of the binary matches the name of the Application.
    #        add_custom_command(TARGET ShaderManager POST_BUILD
    #                COMMAND ln -sf PrusaSlicer prusa-slicer
    #                COMMAND ln -sf PrusaSlicer prusa-gcodeviewer
    #                COMMAND ln -sf PrusaSlicer PrusaGCodeViewer
    #                WORKING_DIRECTORY "$<TARGET_FILE_DIR:PrusaSlicer>"
    #                COMMENT "Symlinking the G-code viewer to PrusaSlicer, symlinking to prusa-slicer and prusa-gcodeviewer"
    #                VERBATIM)
    #    else ()
    #        add_custom_command(TARGET PrusaSlicer POST_BUILD
    #                COMMAND ln -sf prusa-slicer prusa-gcodeviewer
    #                WORKING_DIRECTORY "$<TARGET_FILE_DIR:PrusaSlicer>"
    #                COMMENT "Symlinking the G-code viewer to PrusaSlicer"
    #                VERBATIM)
    #    endif ()
    if (XCODE)
        # Because of Debug/Release/etc. configurations (similar to MSVC) the slic3r binary is located in an extra level
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")
    else ()
        set(BIN_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/../resources")
    endif ()
    add_custom_command(TARGET slic3r-app-wxtest POST_BUILD
            COMMAND ln -sfn "${SLIC3R_RESOURCES_NEXT_DIR}" "${BIN_RESOURCES_DIR}"
            COMMENT "Symlinking the resources directory into the build tree"
            VERBATIM)
endif ()
