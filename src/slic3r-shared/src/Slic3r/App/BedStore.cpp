#include "Slic3r/App/BedStore.hpp"

static Slic3r::App::BedStore g_bed_store;

namespace Slic3r::App {

const Slic3r::App::BedStore& bed_store()
{
    return g_bed_store;
}

void set_bed_store(const Slic3r::App::BedStore& store)
{
    g_bed_store = store;
}

void clear_bed_store()
{
    g_bed_store.beds.clear();
}

} // namespace Slic3r::App
