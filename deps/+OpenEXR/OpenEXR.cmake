add_cmake_project(OpenEXR
    # GIT_REPOSITORY https://github.com/openexr/openexr.git
    URL https://github.com/AcademySoftwareFoundation/openexr/releases/download/v3.2.4/openexr-v3.2.4.tar.gz
    URL_HASH SHA256=0ad76308342bf8c08e55f5e53d685c4fc79f8a4e25924e6ab1d19d4f7e178d14
    GIT_TAG v3.2.4
    CMAKE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_TESTING=OFF 
        -DPYILMBASE_ENABLE:BOOL=OFF 
        -DOPENEXR_VIEWERS_ENABLE:BOOL=OFF
        -DOPENEXR_BUILD_UTILS:BOOL=OFF
        -DOPENEXR_BUILD_TOOLS:BOOL=OFF
        -DOPENEXR_BUILD_EXAMPLES:BOOL=OFF
        -DBUILD_WEBSITE:BOOL=OFF
        -DOPENEXR_IS_SUBPROJECT:BOOL=ON
    EMSCRIPTEN_CMAKE_ARGS
        -DCMAKE_CXX_FLAGS=--use-port=zlib
        -DCMAKE_C_FLAGS=--use-port=zlib
)

set(DEP_OpenEXR_DEPENDS ZLIB libdeflate Imath)
