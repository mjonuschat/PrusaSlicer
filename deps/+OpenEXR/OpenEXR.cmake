add_cmake_project(OpenEXR
    # GIT_REPOSITORY https://github.com/openexr/openexr.git
    URL https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v2.5.5.zip
    URL_HASH SHA256=0307a3d7e1fa1e77e9d84d7e9a8694583fbbbfd50bdc6884e2c96b8ef6b902de
    GIT_TAG v2.5.5
    PATCH_COMMAND COMMAND ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/OpenEXR.patch
    CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_TESTING=OFF 
        -DPYILMBASE_ENABLE:BOOL=OFF 
        -DOPENEXR_VIEWERS_ENABLE:BOOL=OFF
        -DOPENEXR_BUILD_UTILS:BOOL=OFF
        -DINSTALL_OPENEXR_EXAMPLES:BOOL=OFF
    EMSCRIPTEN_CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=--use-port=zlib
        -DCMAKE_C_FLAGS=--use-port=zlib
)

set(DEP_OpenEXR_DEPENDS ZLIB)
