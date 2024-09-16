#pragma once

#include <string>

namespace Slic3r {

inline const std::string& L(const std::string& s)                                   { return s; }       // Mark, do NOT translate
inline const std::string& L_CONTEXT(const std::string& s, const std::string& ctx)   { return s; }       // Mark, do NOT translate

extern std::string _u8(const std::string& s);                                                           // Translate, do NOT mark.
inline std::string _u8L(const std::string& s)                                       { return _u8(s); }  // Mark and translate.

extern std::string _CTX_utf8(const std::string& s, const std::string& ctx);                             // Mark and translate.
extern std::string _L_PLURAL_u8(const std::string& single, const std::string& plural, int n);           // Mark and translate.

}