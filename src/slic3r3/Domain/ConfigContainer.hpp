#pragma once

#include "libslic3r/ObjectID.hpp"

#include <memory>
#include <vector>

#include "libslic3r/PrintConfig.hpp"

#include <CGAL/Object.h>

namespace Slic3r::Domain {
class Bed;

class ConfigContainer : public ObjectBase
{
public:

private:
    using BedList = std::vector<std::unique_ptr<Bed>>;

    PrinterTechnology m_print_technology {ptFFF};
    DynamicPrintConfig m_print_config;
    BedList m_beds;
};

} // namespace Slic3r::Domain
