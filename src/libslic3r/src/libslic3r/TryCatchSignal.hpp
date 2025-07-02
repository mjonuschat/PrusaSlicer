///|/ Copyright (c) Prusa Research 2021 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef TRY_CATCH_SIGNAL_HPP
#define TRY_CATCH_SIGNAL_HPP

#ifdef _MSC_VER
#include "TryCatchSignalSEH.hpp"
#else

#include <csignal>

using SignalT = decltype (SIGSEGV);

template<class TryFn, class CatchFn, int N>
void try_catch_signal(const SignalT (&/*sigs*/)[N], TryFn &&fn, CatchFn &&/*cfn*/)
{
    fn();
}
#endif

#endif // TRY_CATCH_SIGNAL_HPP

