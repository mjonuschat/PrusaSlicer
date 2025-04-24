#include <functional>
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::Domain::Preset {

PresetValueMap build_features(const FeatureDefs& feature_defs)
{
    PresetValueMap result;
    for (const auto& [name, def] : feature_defs)
        result[name] = def.default_value;
    return result;
}

void override_features(PresetValueMap& dest, const FeatureDefs& overrides)
{
    for (const auto& [k, v] : overrides)
        dest[k] = v.default_value;
}

void override_features(PresetValueMap& dest, const PresetValueMap& overrides)
{
    for (const auto& [k, v] : overrides)
        dest[k] = v;
}

namespace {
template <typename T>
const T* find_element_by_id(
    const VendorData& vendor_data,
    const std::string& id,
    const std::function<const std::map<std::string, T>& (const HwPrinterTechnologyDefs&)>&
        element_container_getter
)
{
    for (const auto& [_, pt_defs] : vendor_data.defs) {
        const auto& elements = element_container_getter(pt_defs);
        if (const auto it = elements.find(id); it != elements.end())
            return &it->second;
    }
    return nullptr;

}
}

const HwPrinterConfigDef* VendorData::find_printer_config_def_by_id(const std::string& id) const
{
    return find_element_by_id<HwPrinterConfigDef>(
        *this,
        id,
        [](const HwPrinterTechnologyDefs& pt_defs) -> const auto& {
            return pt_defs.printers;
        }
    );
}

const HwToolConfigDef* VendorData::find_tool_config_def_by_id(const std::string& id) const
{
    return find_element_by_id<HwToolConfigDef>(
        *this,
        id,
        [](const HwPrinterTechnologyDefs& pt_defs) -> const auto& {
            return pt_defs.tools;
        }
    );
}

const HwFeederConfigDef* VendorData::find_feeder_config_def_by_id(const std::string& id) const
{
    return find_element_by_id<HwFeederConfigDef>(
        *this,
        id,
        [](const HwPrinterTechnologyDefs& pt_defs) -> const auto& {
            return pt_defs.feeders;
        }
    );
}

const HwPrinterConfigTemplate* VendorData::find_printer_config_template_by_id(const std::string& id) const
{
    auto it = std::find_if(printer_configs.begin(), printer_configs.end(), [&](const auto& pc){ return pc.id == id; });
    return it == printer_configs.end() ? nullptr : &*it;
}




MaterialIterator::MaterialIterator(const HwPrinterConfig& config)
    : m_config(config), m_current_address({0})
{
    if (!m_config.feeders.empty())
        descent_to_material_slot();
}

void MaterialIterator::descent_to_material_slot()
{
    ASSERT(!m_current_address.empty());
    const HwFeederConfig* feeder = nullptr;
    do {
        const auto it = m_config.feeders.find(m_current_address);
        m_current_address.push_back(0);
        feeder = it == m_config.feeders.end() ? nullptr : &it->second;
    } while (feeder != nullptr);
    m_current_address.pop_back();
}

MaterialIterator& MaterialIterator::operator++()
{
    if (m_current_address.back() + 1 < current_slot_count()) {
        m_current_address.back()++;
        descent_to_material_slot();
    } else {
        // move up till the next free slot is available
        while (!m_current_address.empty() && m_current_address.back() + 1 >= current_slot_count())
            m_current_address.pop_back();
        // if there is still valid address
        if (!m_current_address.empty()) {
            // increment address
            m_current_address.back()++;
            // move down to first children till material slot reached
            descent_to_material_slot();
        }
    }
    return *this;
}

size_t MaterialIterator::current_slot_count() const
{
    size_t n = m_current_address.size();
    if (n == 0)
        return 0;
    if (n == 1)
        return m_config.tool_count;
    auto it = m_config.feeders.find({m_current_address.begin(), m_current_address.begin() + (n - 1)});
    ASSERT(it != m_config.feeders.end());
    return it->second.slot_count;
}



}
