#include "Slic3r/Domain/BedInstance.hpp"

namespace Slic3r::Domain {

BedInstance::BedInstance(const Bed& bed) : bed(bed) {}

size_t BedInstance::index() const
{
    return m_index;
}

void BedInstance::set_index(size_t index)
{
    if (m_index != index) {
        m_index = index;
        m_label = std::to_string(index);
        m_name  = "Bed " + m_label;
    }
}

const std::string& BedInstance::label() const
{
    return m_label;
}

const std::string& BedInstance::name() const
{
    return m_name;
}

} // namespace Slic3r::Domain
