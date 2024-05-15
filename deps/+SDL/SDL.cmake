include(GNUInstallDirs)
add_cmake_project(
        SDL
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-2.30.2.zip
        URL_HASH SHA256=09a822abf6e97f80d09cf9c46115faebb3476b0d56c1c035aec8ec3f88382ae7
        CMAKE_ARGS
            -DINCLUDE_INSTALL_DIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DBUILD_SHARED_LIBS=ON
            -DCMAKE_BUILD_SHARED_LIBS=ON
        EMSCRIPTEN_PORT sdl2
)
