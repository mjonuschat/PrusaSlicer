set(TBB_URL "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.5.0.zip")
set(TBB_URL_HASH SHA256=83ea786c964a384dd72534f9854b419716f412f9d43c0be88d41874763e7bb47)

if (EMSCRIPTEN)
    set(TBB_URL "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.12.0.zip")
    set(TBB_URL_HASH SHA256=fe6ca052b5bdd2c6e0616b360c9b0dcbcc46e01bbd0aa8fd0517c17fc58931db)
endif()

add_cmake_project(
    TBB
    URL ${TBB_URL}
    URL_HASH ${TBB_URL_HASH}
    CMAKE_ARGS
        -DTBB_BUILD_SHARED=${BUILD_SHARED_LIBS}
        -DTBB_TEST=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_DEBUG_POSTFIX=_debug
)


