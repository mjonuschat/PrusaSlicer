add_cmake_project(cpptrace
    URL "https://github.com/jeremy-rifkin/cpptrace/archive/refs/tags/v0.7.4.zip"
    URL_HASH SHA256=a81d751c9301fa7d6b940a2832a4a3ca81f4cb8d4194134be11e4150ed588cf3
    PATCH_COMMAND ${PATCH_CMD} ${CMAKE_CURRENT_LIST_DIR}/cpptrace.patch
    CMAKE_ARGS
        -DCPPTRACE_USE_EXTERNAL_ZSTD=ON
        -DCPPTRACE_USE_EXTERNAL_LIBDWARF=ON
)

set(DEP_cpptrace_DEPENDS zstd libdwarf)
