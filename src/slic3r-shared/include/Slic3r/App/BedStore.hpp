#pragma once

#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Biz/Slicing/BackgroundProcess.hpp"

namespace Slic3r::App {

struct BedStore
{
    Domain::BedRefs beds;
};

const Slic3r::App::BedStore& bed_store();
void set_bed_store(const Slic3r::App::BedStore& store);
void clear_bed_store();

} // namespace Slic3r::App
