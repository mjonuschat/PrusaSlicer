///|/ Copyright (c) Prusa Research 2026 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#pragma once
#include "Slic3r/App/Scene/Ray.hpp"
#include "Slic3r/App/Scene/Scene.hpp" // NodePickResults
#include "Slic3r/Biz/Emboss/EmbossJob.hpp" // CreateVolumeParams

namespace Slic3r::App::Scene {
/**
@brief Create new volume on position of mouse cursor
@param input Cantain all needed data for start creation job
@param pick_ray Ray into scene given by coordinate on screen
@param picks Scene Node with intersection of picked ray
@return True on success otherwise False
*/
bool start_create(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks);
bool start_create_volume(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks);
bool start_create_object(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks);

} // namespace Slic3r::App::Scene
