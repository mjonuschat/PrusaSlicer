#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/Bed.hpp"

namespace Slic3r::Domain {

BedInstance& ConfigContainer::add_bed_instance()
{
    ASSERT(m_bed != nullptr, "ConfigContainer's Bed is null");
    m_bed_instances.emplace_back(std::make_unique<BedInstance>(*m_bed));
    return *m_bed_instances.back();
}

void ConfigContainer::remove_last_bed_instance()
{
    m_bed_instances.pop_back();
}

void ConfigContainer::remove_bed_instance_by_id(size_t id)
{
    auto it = std::find_if(m_bed_instances.begin(), m_bed_instances.end(),
        [id](const auto& i) { return i->id().id == id; });
    if (it != m_bed_instances.end())
        m_bed_instances.erase(it);
}

void ConfigContainer::clear_bed_instances()
{
    m_bed_instances.clear();
}

} // namespace Slic3r::Domain
