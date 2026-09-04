# Disable Boost's optional ICU auto-detection during the deps build.
# CI runners never have ICU installed, so Boost::regex is built without
# ICU support there and nothing downstream ever needs icudata at link
# time. On a dev machine with Homebrew's icu4c present, Boost's own
# CMake configure auto-detects it and builds an ICU-linked regex,
# which then needs a matching icudata (and its own zstd dependency)
# on the linker search path. Disabling detection here matches CI's
# actual build shape instead of teaching the linker to find a library
# CI never links against.
set(CMAKE_DISABLE_FIND_PACKAGE_ICU ON CACHE BOOL "" FORCE)
