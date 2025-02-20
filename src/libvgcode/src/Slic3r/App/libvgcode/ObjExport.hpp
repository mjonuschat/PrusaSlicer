///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/libvgcode/Types.hpp"

#include <stdio.h>

namespace Slic3r::App::libvgcode {

class ViewerImpl;

bool export_toolpaths_to_obj(FILE& obj_file, FILE& mtl_file, const ObjExportParams& params, const ViewerImpl& viewer);

} // namespace Slic3r::App::libvgcode
