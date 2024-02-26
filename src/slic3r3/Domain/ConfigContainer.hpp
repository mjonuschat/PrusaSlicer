#pragma once

#include <memory>
#include <vector>

#include "libslic3r/PrintConfig.hpp"


namespace Slic3r::Domain {
class Bed;

class ConfigContainer
{
public:
private:
    using BedList = std::vector<std::unique_ptr<Bed>>;

    DynamicPrintConfig m_print_config;
    BedList m_beds;
};

} // namespace Slic3r::Domain
