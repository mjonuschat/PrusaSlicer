#pragma once

#include "libslic3r/ObjectID.hpp"

#include <memory>
#include <vector>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"

#include <CGAL/Object.h>

namespace Slic3r::Domain {

class ConfigContainer : public ObjectBase
{
public:

    PrinterTechnology print_technology() const { return m_print_technology; }
    const DynamicPrintConfig& get_print_config() const { return m_print_config; }
    void set_print_config(const DynamicPrintConfig& config) {
        m_print_config = config;
        m_print_technology = Preset::printer_technology(m_print_config);
    }

private:
    PrinterTechnology m_print_technology {ptFFF};
    DynamicPrintConfig m_print_config;
};

} // namespace Slic3r::Domain
