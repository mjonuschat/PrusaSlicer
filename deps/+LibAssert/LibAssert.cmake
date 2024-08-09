add_cmake_project(LibAssert
        URL https://github.com/jeremy-rifkin/libassert/archive/refs/tags/v2.1.0.zip
        URL_HASH SHA256=6fda4c371e515aa9925ddd37cf4b1c57ec40042b8489470e5458ee30576778e3
        EMSCRIPTEN_EXCLUDED ON
#        EMSCRIPTEN_CMAKE_ARGS
#        -DCPPTRACE_UNWIND_WITH_NOTHING=ON
#        -DCPPTRACE_GET_SYMBOLS_WITH_NOTHING=ON
)
