#pragma once
/**
 * Assertion API
 * - ASSERT(x), ASSERT_VAL(x) enforces condition in both debug AND release configurations (!)
 * - DEBUG_ASSERT(x), DEBUG_ASSERT_VAL(x) enforces condition only in debug configuration.
 * For more details see [libassert](https://github.com/jeremy-rifkin/libassert)
 */
#ifndef __EMSCRIPTEN__

#include <libassert/assert.hpp>

#else // #ifndef __EMSCRIPTEN__

// As we cannot use libassert in emscripten follows minimal implementation of the libassert API:

#include <iostream>


#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define ASSERT(x) if (!(x)) { \
        std::cerr << "Assertion " << #x << " failed in " << __FILE__ << ":" << __LINE__ \
                  << " (function " << __PRETTY_FUNCTION__ << ")\n"; \
        std::abort(); \
    }

#define ASSERT_VAL(x) ::Slic3r::assert_val(x, __FILE__, __LINE__, __PRETTY_FUNCTION__,  #x)

namespace Slic3r {

template<typename T>
inline T assert_val(T expr, const char* filename, size_t line, const char* func, const char* expr_str)
{
    if (!(expr)) {
        std::cerr << "Assertion " << expr_str << " failed in " << filename << ":" << line
                  << " (function " << func << ")\n";
        std::abort();
    }
    return expr;
}

}

#ifdef NDEBUG

#define DEBUG_ASSERT(x)
#define DEBUG_ASSERT_VAL(x) (x)

#else // #ifdef NDEBUG

#define DEBUG_ASSERT(x) ASSERT(x)
#define DEBUG_ASSERT_VAL(x) ASSERT_VAL(x)

#endif // #ifdef NDEBUG

#endif // #ifndef __EMSCRIPTEN__

