set(LibBGCode_SOURCE_DIR "" CACHE PATH "Optionally specify local LibBGCode source directory")

set(_source_dir_line
    URL https://github.com/prusa3d/libbgcode/archive/c019e7863bd213280f2624c541987cf05867d871.zip
    URL_HASH SHA256=e91e5989094bc315f8843a353f81ad18e3316add20393886cbfa0324ec469503)

if (LibBGCode_SOURCE_DIR)
    set(_source_dir_line "SOURCE_DIR;${LibBGCode_SOURCE_DIR};BUILD_ALWAYS;ON")
endif ()

# add_cmake_project(LibBGCode_deps
#     ${_source_dir_line}
#     SOURCE_SUBDIR deps
#     BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/
#     CMAKE_ARGS
#         -DLibBGCode_Deps_DEP_DOWNLOAD_DIR:PATH=${${PROJECT_NAME}_DEP_DOWNLOAD_DIR}
#         -DDEP_CMAKE_OPTS:STRING=-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
#         -DLibBGCode_Deps_SELECT_ALL:BOOL=OFF
#         -DLibBGCode_Deps_SELECT_heatshrink:BOOL=ON
#         -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
#         -DLibBGCode_Deps_DEP_INSTALL_PREFIX=${${PROJECT_NAME}_DEP_INSTALL_PREFIX}
# )

add_cmake_project(LibBGCode
    ${_source_dir_line}
    CMAKE_ARGS
        -DLibBGCode_BUILD_TESTS:BOOL=OFF
        -DLibBGCode_BUILD_CMD_TOOL:BOOL=OFF
        -DCMAKE_FIND_ROOT_PATH=/
)

# set(DEP_LibBGCode_deps_DEPENDS ZLIB Boost)
# set(DEP_LibBGCode_DEPENDS LibBGCode_deps)
if (EMSCRIPTEN)
    set(DEP_LibBGCode_DEPENDS heatshrink)
else ()
    set(DEP_LibBGCode_DEPENDS ZLIB Boost heatshrink)
endif ()
