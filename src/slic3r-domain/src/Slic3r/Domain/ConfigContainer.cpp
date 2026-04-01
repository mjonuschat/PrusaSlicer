#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Bed.hpp"

namespace Slic3r::Domain {

ConfigPack ConfigContainer::build_print_config() const
{
    ConfigPack pack = m_preset.config();
    if (auto* fdm = std::get_if<ConfigPackFDM>(&pack)) {
        fdm->project = m_project_settings;
    }
    return pack;
}

BedInstance& ConfigContainer::add_bed_instance()
{
    ASSERT(m_bed != nullptr, "ConfigContainer's Bed is null");
    m_bed_instances.emplace_back(std::make_unique<BedInstance>(*m_bed));
    return *m_bed_instances.back();
}

ConfigContainer::BedInstanceList::const_iterator ConfigContainer::remove_bed_instance_by_id(size_t id)
{
    auto it = std::find_if(m_bed_instances.begin(), m_bed_instances.end(),
        [id](const auto& i) { return i->id().id == id; });
    if (it != m_bed_instances.end())
        return m_bed_instances.erase(it);
    return m_bed_instances.end();
}


} // namespace Slic3r::Domain
