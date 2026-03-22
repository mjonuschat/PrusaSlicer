# Toolchain overlay: add /FS for Ninja + MSVC parallel PDB writes
# Loaded via CMAKE_TOOLCHAIN_FILE, uses FLAGS_INIT to append rather than replace defaults.
string(APPEND CMAKE_C_FLAGS_INIT " /FS")
string(APPEND CMAKE_CXX_FLAGS_INIT " /FS")
