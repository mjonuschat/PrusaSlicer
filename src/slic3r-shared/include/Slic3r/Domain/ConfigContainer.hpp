#pragma once

#include "FindById.hpp"
#include "libslic3r/ObjectID.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include <memory>
#include <vector>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"

namespace Slic3r::Domain {

class ConfigContainer : public ObjectBase
{
public:
    using BedInstanceList = std::vector<BedInstance*>;

    PrinterTechnology print_technology() const { return m_print_technology; }
    const DynamicPrintConfig& print_config() const { return m_print_config; }
    void set_print_config(const DynamicPrintConfig& config)
    {
        m_print_config = config;
        m_print_technology = Preset::printer_technology(m_print_config);
    }

    BedInstanceList& bed_instances() { return m_bed_instances; }
    const BedInstanceList& bed_instances() const { return m_bed_instances; }

    const BedInstance& find_bed_instance_by_id(size_t id) const
    { return *DEBUG_ASSERT_VAL(find_by_id(m_bed_instances, id)); }

    BedInstance& find_bed_instance_by_id(size_t id)
    { return *DEBUG_ASSERT_VAL(find_by_id(m_bed_instances, id)); }

private:
    PrinterTechnology m_print_technology {ptFFF};
    /**
     * @brief Full config as loaded from 3MF.
     *
     * During editing this config gets parsed and decomposed into Preset stored in
     * Slic3r::Biz::Preset::PresetInteractorConfigContainerContext.
     */
    DynamicPrintConfig m_print_config;

    BedInstanceList m_bed_instances;
};

} // namespace Slic3r::Domain
