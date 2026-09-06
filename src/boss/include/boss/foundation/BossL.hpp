// src/boss/include/boss/foundation/BossL.hpp
//
// Translation-extraction marker for BOSS code. xgettext collects the
// literals passed to it -- see the --keyword list in
// cmake/modules/Localization.cmake. It mirrors ConfigDefsFDM.cpp's
// file-local L(), which no other translation unit can reach. The name
// differs from L() so a file that already defines its own L() can include
// this header without a conflict.
#pragma once

#include <string>

namespace Slic3r::Boss {

inline std::string BossL(const std::string& s) { return s; }

} // namespace Slic3r::Boss
