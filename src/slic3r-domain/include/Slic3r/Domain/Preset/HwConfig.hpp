#pragma once

#include <variant>
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <iterator>
#include <cstddef>

#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Domain/Preset/SourceLocatedExpr.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::Domain::Preset {

/**
 * @brief Address of feeder or material slot.
 *
 * The address describes position of particular element in feeder topology.
 *
 * Examples:
 * - `{0}`: denotes first tool slot
 * - `{2,0}`: denotes a first material slot in a feeder attached to the 3rd tool head.
 * .
 */
using Address = std::vector<uint8_t>;

/**
 * @brief Convert address from Slicer internal (zero based) to public (one based)
 * @param address Internal address representation
 * @return Public address representation
 */
uint8_t address_to_public(uint8_t address);

/**
 * @brief Convert address from Slicer internal (zero based) to public (one based)
 * @param address Internal address representation
 * @return Public address representation
 */
Address address_to_public(const Address& address);

/**
 * @brief Convert address from public (one based) to Slicer internal (zero based)
 * @param address Public address representation
 * @return Internal address representation
 */
uint8_t address_from_public(uint8_t address);

/**
 * @brief Convert address from public (one based) to Slicer internal (zero based)
 * @param address Public address representation
 * @return Internal address representation
 */
Address address_from_public(const Address& address);

/**
 * @brief Model information of a hardware component (like printer or feeder).
 */
struct HwModel
{
    std::string model;
    std::string base_model;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(model, base_model);
    }

    friend bool operator==(const HwModel& lhs, const HwModel& rhs)
    {
        return lhs.model == rhs.model && lhs.base_model == rhs.base_model;
    }

    friend bool operator!=(const HwModel& lhs, const HwModel& rhs)
    {
        return !(lhs == rhs);
    }
};

/**
 * @brief Type of tool head.
 */
enum class ToolType : uint8_t
{
    FdmPrintTool,
};

/**
 * @brief Tool head configuration
 */
struct HwToolConfig
{
    std::string id;
    std::string name;
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, features);
    }

    friend bool operator==(const HwToolConfig& lhs, const HwToolConfig& rhs)
    {
        return lhs.id == rhs.id && lhs.name == rhs.name && lhs.features == rhs.features;
    }

    friend bool operator!=(const HwToolConfig& lhs, const HwToolConfig& rhs)
    {
        return !(lhs == rhs);
    }
};

/**
 * @brief Type of feeder
 */
enum class FeederType : uint8_t
{
    Manual, ///< User feel feed filaments manually on prompt.
    MMU
};

struct HwFeederConfig
{
    std::string id;
    FeederType type;
    HwModel model;
    uint32_t slot_count{0};
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, type, model, slot_count, features);
    }

    friend bool operator==(const HwFeederConfig& lhs, const HwFeederConfig& rhs)
    {
        return lhs.id == rhs.id && lhs.type == rhs.type && lhs.model == rhs.model && lhs.slot_count == rhs.slot_count && lhs.features == rhs.features;
    }

    friend bool operator!=(const HwFeederConfig& lhs, const HwFeederConfig& rhs)
    {
        return !(lhs == rhs);
    }
};

struct MaterialConfig
{
    std::string id;
    std::optional<std::string> type;
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, type, features);
    }

    friend bool operator==(const MaterialConfig& lhs, const MaterialConfig& rhs)
    {
        return lhs.id == rhs.id && lhs.type == rhs.type && lhs.features == rhs.features;
    }

    friend bool operator!=(const MaterialConfig& lhs, const MaterialConfig& rhs)
    {
        return !(lhs == rhs);
    }
};

struct HwSheetConfig
{
    std::string id;
    std::string name;
    std::string type;
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, type, features);
    }

    friend bool operator==(const HwSheetConfig& lhs, const HwSheetConfig& rhs)
    {
        return lhs.id == rhs.id && lhs.name == rhs.name && lhs.type == rhs.type && lhs.features == rhs.features;
    }

    friend bool operator!=(const HwSheetConfig& lhs, const HwSheetConfig& rhs)
    {
        return !(lhs == rhs);
    }
};

using HwToolConfigs     = std::vector<HwToolConfig>;
using HwFeederConfigs   = std::map<Address, HwFeederConfig>;
using HwMaterialConfigs = std::map<Address, MaterialConfig>;

struct VisualRepresentation
{
    std::optional<std::string> bed_model;
    std::optional<std::string> bed_texture;
    std::optional<std::string> thumbnail;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(bed_model, bed_texture, thumbnail);
    }

    friend bool operator==(const VisualRepresentation& lhs, const VisualRepresentation& rhs)
    {
        return lhs.bed_model == rhs.bed_model && lhs.bed_texture == rhs.bed_texture && lhs.thumbnail == rhs.thumbnail;
    }

    friend bool operator!=(const VisualRepresentation& lhs, const VisualRepresentation& rhs)
    {
        return !(lhs == rhs);
    }
};

struct HwPrinterConfig
{
    std::string id;
    std::string printer_id;
    std::string vendor_id;
    std::string repo_id;
    std::string repo_version;
    std::string name;
    PrinterTechnology technology;
    HwModel model;
    uint8_t tool_count;
    FeatureValueMap features;

    // visual representation
    VisualRepresentation visual;

    HwToolConfigs tools;
    HwFeederConfigs feeders;
    HwMaterialConfigs materials;
    HwSheetConfig sheet;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, printer_id, vendor_id, repo_id, repo_version, name, technology,
            model, tool_count, features, visual, tools, feeders, materials, sheet);
    }

    std::string relative_path_to_assets() const;

    bool has_same_values(const HwPrinterConfig& other) const;
};

using HwPrinterConfigs = std::vector<HwPrinterConfig>;

struct FeatureDef
{
    FeatureValue default_value;
    std::vector<FeatureValue> allowed_values;
    bool user_editable{true};

    template<class Archive> void serialize(Archive& archive)
    {
        archive(default_value, allowed_values, user_editable);
    }
};

using FeatureDefs = std::map<std::string, FeatureDef>;

FeatureValueMap build_features(const FeatureDefs& feature_defs);

void override_features(FeatureValueMap& dest, const FeatureDefs& overrides);
void override_features(FeatureValueMap& dest, const FeatureValueMap& overrides);
void override_visual(VisualRepresentation& dest, const VisualRepresentation& overrides);

void fill_missing_features_with_default(FeatureValueMap& dest, const FeatureDefs& def);
void remove_features_with_default(FeatureValueMap& features, const FeatureDefs& def_features);

struct HwPrinterConfigDef
{
    std::string id;
    std::string name;
    PrinterTechnology technology;
    HwModel model;
    std::vector<std::string> legacy_printer_model;
    FeatureDefs features;
    uint8_t tool_count;
    VisualRepresentation visual;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, technology, model, features, legacy_printer_model, tool_count, visual);
    }
};

struct HwToolConfigDef
{
    std::string id;
    std::string name;
    PrinterTechnology technology;
    std::optional<SourceLocatedExpr> condition;
    FeatureDefs features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, technology, condition, features);
    }
};

struct HwFeederConfigDef
{
    std::string id;
    std::string name;
    PrinterTechnology technology;
    FeederType type;
    std::optional<SourceLocatedExpr> condition;
    HwModel model;
    FeatureDefs features;
    uint8_t slot_count;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, technology, type, condition, model, features, slot_count);
    }
};

struct HwSheetConfigDef
{
    std::string id;
    std::string name;
    std::string type;
    std::optional<SourceLocatedExpr> condition;
    FeatureDefs features;

    template<class Archive> void serialize(Archive& archive){
        archive(id, name, type, condition, features);
    }
};

struct HwPrinterTechnologyDefs
{
    PrinterTechnology technology;
    std::map<std::string, HwPrinterConfigDef> printers;
    std::map<std::string, HwToolConfigDef> tools;
    std::map<std::string, HwFeederConfigDef> feeders;
    std::map<std::string, HwSheetConfigDef> sheets;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(technology, printers, tools, feeders, sheets);
    }
};

using HwDefs = std::map<PrinterTechnology, HwPrinterTechnologyDefs>;

struct HwToolConfigTemplate
{
    std::string id;
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, features);
    }
};

using HwToolConfigTemplates = std::vector<HwToolConfigTemplate>;

struct HwFeederConfigTemplate
{
    std::string id;
    Address address;
    FeatureValueMap features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, address, features);
    }
};

using HwFeederConfigTemplates = std::vector<HwFeederConfigTemplate>;

struct HwPrinterConfigTemplate
{
    std::string id;
    std::string name;
    std::string printer;
    std::vector<std::string> legacy_printer_model;
    std::optional<std::string> sheet;
    std::optional<uint8_t> tool_count;
    FeatureValueMap features;
    HwToolConfigTemplates tools;
    HwFeederConfigTemplates feeders;
    VisualRepresentation visual;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, name, printer, legacy_printer_model, sheet, tool_count, features, tools, feeders, visual);
    }
};

using HwPrinterConfigTemplates = std::vector<HwPrinterConfigTemplate>;

struct VendorFeatures
{
    FeatureDefs printer;
    FeatureDefs tool;
    FeatureDefs feeder;
    FeatureDefs sheet;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(printer, tool, feeder, sheet);
    }
};

struct VendorInfo
{
    std::string id;
    std::string repo_id;
    std::string name;
    std::string version;
    VendorFeatures features;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(id, repo_id, name, version, features);
    }
};

struct VendorData
{
    VendorInfo info;
    HwPrinterConfigTemplates printer_configs;
    HwDefs defs;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(info, printer_configs, defs);
    }

    const HwPrinterConfigDef* find_printer_config_def_by_id(const std::string& id) const;
    const HwPrinterConfigDef* find_printer_config_def_by_legacy_printer_model(const std::string& id) const;
    const HwToolConfigDef* find_tool_config_def_by_id(const std::string& id) const;
    const HwFeederConfigDef* find_feeder_config_def_by_id(const std::string& id) const;
    const HwSheetConfigDef* find_sheet_config_def_by_id(const std::string& id) const;
    const HwPrinterConfigTemplate* find_printer_config_template_by_id(const std::string& id) const;
    const HwPrinterConfigTemplate* find_printer_config_template_by_legacy_printer_model(const std::string& id) const;
};

class MaterialIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = MaterialIterator;
    using pointer           = value_type*;
    using reference         = value_type&;

    explicit MaterialIterator(const HwPrinterConfig& config);
    MaterialIterator(const MaterialIterator&) = default;

    MaterialIterator& operator++();

    MaterialIterator operator++(int)
    {
        MaterialIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    reference operator*()
    {
        return *this;
    }

    pointer operator->()
    {
        return this;
    }

    bool operator==(const MaterialIterator& rhs) const
    {
        return m_current_address == rhs.m_current_address;
    }

    bool operator!=(const MaterialIterator& rhs) const
    {
        return m_current_address != rhs.m_current_address;
    }

    bool is_valid() const
    {
        return !m_current_address.empty();
    }

    const Address& address() const
    {
        return m_current_address;
    }

    const HwToolConfig& tool_config() const
    {
        ASSERT(is_valid());
        return m_config.tools[m_current_address.front()];
    }

    size_t feeder_count() const
    {
        ASSERT(is_valid());
        return m_current_address.size() - 1;
    }

    Address feeder_address(size_t level) const
    {
        ASSERT(level + 1 < m_current_address.size());
        return Address{m_current_address.begin(), m_current_address.begin() + (level + 1)};
    }

    const HwFeederConfig& feeder_config(size_t level) const
    {
        auto it = m_config.feeders.find(feeder_address(level));
        ASSERT(it != m_config.feeders.end());
        return it->second;
    }

private:
    friend MaterialIterator begin(const MaterialIterator& it);
    friend MaterialIterator end(const MaterialIterator& it);

    MaterialIterator(const HwPrinterConfig& config, const Address& addr) :
        m_config(config),
        m_current_address(addr)
    {}

    MaterialIterator invalid() const
    {
        return MaterialIterator{m_config, {}};
    }

    size_t current_slot_count() const;
    void descent_to_material_slot();

private:
    const HwPrinterConfig& m_config;
    Address m_current_address;
};

inline MaterialIterator begin(const MaterialIterator& it)
{
    return it;
}

inline MaterialIterator end(const MaterialIterator& it)
{
    return it.invalid();
}

void fill_missing_features_with_default(HwPrinterConfig& printer_config, const VendorData& vendor_data);
HwPrinterConfig remove_features_with_default(
    const HwPrinterConfig& printer_config,
    const VendorData& vendor_data
);

std::string suggest_name(const HwPrinterConfig& cfg, const VendorData& vendor_data);

template <typename T>
std::optional<T> get_feature(const Preset::FeatureValueMap& features, const std::string& key)
{
    auto feature_it{features.find(key)};
    if (feature_it == features.end()) {
        return std::nullopt;
    }
    int temp_value{};
    const T* feature{nullptr};
    if constexpr (std::is_same_v<T, int>) {
        temp_value = int(*std::get_if<double>(&feature_it->second));
        feature    = &temp_value;
    } else {
        feature = std::get_if<T>(&feature_it->second);
    }
    if (!feature) {
        return std::nullopt;
    }
    return *feature;
}

} // namespace Slic3r::Domain::Preset
