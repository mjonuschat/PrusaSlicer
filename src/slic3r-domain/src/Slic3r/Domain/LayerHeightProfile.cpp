#include "Slic3r/Domain/LayerHeightProfile.hpp"

#include <cereal/types/base_class.hpp>

namespace Slic3r::Domain {

void LayerHeightProfile::assign(const LayerHeightProfile& rhs)
{
    if (!this->timestamp_matches(rhs)) {
        m_data = rhs.m_data;
        this->copy_timestamp(rhs);
    }
}

void LayerHeightProfile::assign(LayerHeightProfile&& rhs)
{
    if (!this->timestamp_matches(rhs)) {
        m_data = std::move(rhs.m_data);
        this->copy_timestamp(rhs);
    }
}

const std::vector<double>& LayerHeightProfile::get() const noexcept { return m_data; }

bool LayerHeightProfile::empty() const noexcept { return m_data.empty(); }

void LayerHeightProfile::set(const std::vector<double>& data)
{
    if (m_data != data) {
        m_data = data;
        this->touch();
    }
}

void LayerHeightProfile::set(std::vector<double>&& data)
{
    if (m_data != data) {
        m_data = std::move(data);
        this->touch();
    }
}

void LayerHeightProfile::clear()
{
    m_data.clear();
    this->touch();
}

} // namespace Slic3r::Domain
