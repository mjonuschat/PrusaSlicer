#pragma once

#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/FindById.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Preset/SelectedPreset.hpp"

#include <memory>
#include <vector>

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Preset.hpp"

namespace Slic3r::Domain {

class Bed;

class ConfigContainer : public ObjectBase
{
public:
    Domain::PrinterTechnology print_technology() const
    {
        return m_preset.technology();
    }

    const Preset::SelectedPreset& selected_preset() const
    {
        return m_preset;
    }

    Preset::SelectedPreset& mutable_selected_preset()
    {
        return m_preset;
    }

    ConfigPack print_config() const
    {
        return m_preset.config();
    }

    void set_bed(const Bed& bed)
    {
        m_bed = &bed;
    }

    const Bed& bed() const
    {
        return *ASSERT_VAL(m_bed);
    }

    /**
     * @name Bed instances management
     * @{
     */
    using BedInstanceList = std::vector<std::unique_ptr<BedInstance>>;

    [[nodiscard]] BedInstanceList& bed_instances()
    {
        return m_bed_instances;
    }

    [[nodiscard]] const BedInstanceList& bed_instances() const
    {
        return m_bed_instances;
    }

    BedInstance& add_bed_instance();
    void remove_last_bed_instance();
    void remove_bed_instance_by_id(size_t id);
    void clear_bed_instances();

    const BedInstance& find_bed_instance(size_t id) const
    {
        return *DEBUG_ASSERT_VAL(find_by_id(m_bed_instances, id));
    }

    BedInstance& find_bed_instance(size_t id)
    {
        return *DEBUG_ASSERT_VAL(find_by_id(m_bed_instances, id));
    }

private:
    Preset::SelectedPreset m_preset{};

    const Bed* m_bed{nullptr};
    BedInstanceList m_bed_instances;
};

} // namespace Slic3r::Domain
