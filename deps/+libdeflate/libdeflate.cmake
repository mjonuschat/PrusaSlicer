add_cmake_project(libdeflate
        URL "https://github.com/ebiggers/libdeflate/archive/refs/tags/v1.22.zip"
        URL_HASH SHA256=da384a2d96e112c25aca295aea5de5a1e4a6d9fd230bd36f506ffd1a772667a4
        CMAKE_ARGS
        -DLIBDEFLATE_BUILD_SHARED_LIB=OFF
        -DLIBDEFLATE_BUILD_GZIP=OFF
)
