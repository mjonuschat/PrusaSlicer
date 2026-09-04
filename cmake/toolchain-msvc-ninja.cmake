# Toolchain overlay: add /FS for Ninja + MSVC parallel PDB writes
# Loaded via CMAKE_TOOLCHAIN_FILE, uses FLAGS_INIT to append rather than replace defaults.
string(APPEND CMAKE_C_FLAGS_INIT " /FS")
string(APPEND CMAKE_CXX_FLAGS_INIT " /FS")

# Release builds enable LTO/IPO (CMakeLists.txt's check_ipo_supported() block),
# which on MSVC means /GL + /LTCG. Each LTCG link spawns its own memory-heavy
# code-generation pass; running several links at once for large TUs (Wipe.cpp,
# PlaceholderParser.cpp, Eigen headers) exhausts RAM on memory-constrained
# runners (C1002 "compiler is out of heap space", C1083 on a temp file). Cap
# concurrent link jobs to a small pool; compiles still run at full parallelism.
set_property(GLOBAL PROPERTY JOB_POOLS link_pool=2)
set(CMAKE_JOB_POOL_LINK link_pool CACHE STRING "" FORCE)

# Deps never need their own test/benchmark binaries built.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Eigen's blas/lapack subdirectories are added unconditionally (not gated
# by BUILD_TESTING) and always run check_language(Fortran). CI's own
# workflow installs Strawberry Perl for OpenSSL's Perl-based build script,
# and its bundled gfortran/MinGW ld.exe gets found by that probe; it then
# fails a full enable_language(Fortran) check against this MSVC toolchain,
# which is a hard CMake error (not the soft NOTFOUND check_language()
# normally gives). CMake's own CheckLanguage module skips its probe
# entirely whenever CMAKE_Fortran_COMPILER is already a defined cache
# variable, regardless of value, so setting it to empty here (not a bogus
# path, which would still be truthy) makes Eigen's own
# if(CMAKE_Fortran_COMPILER) check correctly see "no compiler" and skip
# enable_language(Fortran) without ever probing for one.
set(CMAKE_Fortran_COMPILER "" CACHE FILEPATH "" FORCE)
