///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Hejl @hejllukas, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <vector>

class wxDialog;
class wxWindow;

namespace Slic3r::App::WX {

void    msw_buttons_rescale(wxDialog* dlg, const int em_unit, const std::vector<int>& btn_ids, double height_koef = 1.);
int     em_unit(wxWindow* win);

}

