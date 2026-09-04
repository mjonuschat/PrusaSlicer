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
