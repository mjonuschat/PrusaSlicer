#include "Slic3r/App/Plater/History.hpp"

#include "Slic3r/App/Yoga/Text.hpp"

namespace Slic3r::App::Plater {

History::History(Item* parent) : Window("history", parent)
{
    set_min_size({330.f, 0.f});
    new Yoga::Text("Action history", this);
}

} // namespace Slic3r::App::Plater
