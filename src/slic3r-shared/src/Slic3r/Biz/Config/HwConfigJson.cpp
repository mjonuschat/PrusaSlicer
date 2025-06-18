#include "Slic3r/Biz/Config/HwConfigJson.hpp"
#include <charconv>
#include <tl/expected.hpp>
#include <variant>
#include "Slic3r/Biz/Config/ConfigJson.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/Types.hpp"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "magic_enum/magic_enum.hpp"

using nlohmann::ordered_json;

using Slic3r::Domain::Preset::HwFeederConfig;
using Slic3r::Domain::Preset::HwFeederConfigs;
using Slic3r::Domain::Preset::HwMaterialConfigs;
using Slic3r::Domain::Preset::MaterialConfig;

namespace Slic3r::Domain::Preset {
void to_json(ordered_json& j, const HwFeederConfig& v);
void from_json(const ordered_json& j, HwFeederConfig& v);
void to_json(ordered_json& j, const MaterialConfig& v);
void from_json(const ordered_json& j, MaterialConfig& v);
} // namespace Slic3r::Domain::Preset

namespace {
using PartialyParsedFeederConfigs = std::map<std::string, HwFeederConfig>;
using PartialyParsedMaterialConfigs = std::map<std::string, MaterialConfig>;

using Slic3r::Domain::Preset::Address;

std::string to_string(const Address& v)
{
    return fmt::to_string(
        fmt::join(v | std::views::transform([](auto slot) { return int(slot); }), ".")
    );
}

tl::expected<Address, std::string> from_string(const std::string& input)
{
    Address address;
    for (const auto& slot : std::views::split(input, ".")) {
        int slot_v;
        auto result = std::from_chars(
            std::to_address(slot.begin()),
            std::to_address(slot.end()),
            slot_v,
            10
        );
        if (result.ec != std::errc()) {
            return tl::unexpected{"Failed to parse a number!"};
        }
        address.push_back(slot_v);
    }

    if (address.empty()) {
        return tl::unexpected{"No numbers were parsed!"};
    }

    return address;
}
} // namespace

NLOHMANN_JSON_NAMESPACE_BEGIN

using Slic3r::Domain::Preset::FeatureValue;

template<>
struct adl_serializer<FeatureValue>
{
    static void to_json(ordered_json& j, const FeatureValue& v)
    {
        std::visit([&j](auto&& arg) { j = arg; }, v);
    }

    static void from_json(const ordered_json& j, FeatureValue& v)
    {
        if (j.is_boolean())
            v = j.get<bool>();
        else if (j.is_number())
            v = j.get<float>();
        else if (j.is_string())
            v = j.get<std::string>();
    }
};

using Slic3r::Domain::Preset::HwFeederConfig;
using Slic3r::Domain::Preset::HwFeederConfigs;
using Slic3r::Domain::Preset::HwMaterialConfigs;
using Slic3r::Domain::Preset::MaterialConfig;

template<>
struct adl_serializer<HwFeederConfigs>
{
    static void to_json(ordered_json& j, const HwFeederConfigs& v)
    {
        for (const auto& [key, value] : v) {
            j[::to_string(key)] = value;
        }
    }
};

template<>
struct adl_serializer<PartialyParsedFeederConfigs>
{
    static void from_json(const ordered_json& j, PartialyParsedFeederConfigs& v)
    {
        v.clear();
        for (const auto& [key, value] : j.items()) {
            v.insert({key, value.get<HwFeederConfig>()});
        }
    }
};

template<>
struct adl_serializer<HwMaterialConfigs>
{
    static void to_json(ordered_json& j, const HwMaterialConfigs& v)
    {
        for (const auto& [key, value] : v) {
            j[::to_string(key)] = value;
        }
    }
};

template<>
struct adl_serializer<PartialyParsedMaterialConfigs>
{
    static void from_json(const ordered_json& j, PartialyParsedMaterialConfigs& v)
    {
        v.clear();
        for (const auto& [key, value] : j.items()) {
            v.insert({key, value.get<MaterialConfig>()});
        }
    }
};

NLOHMANN_JSON_NAMESPACE_END

namespace Slic3r::Domain::Preset {

void to_json(ordered_json& j, const HwToolConfig& v)
{
    j = ordered_json{{"id", v.id}, {"features", v.features}};
}

void from_json(const ordered_json& j, HwToolConfig& v)
{
    j.at("id").get_to(v.id);
    j.at("features").get_to(v.features);
}

void to_json(ordered_json& j, const HwModel& v)
{
    j = ordered_json{{"base_model", v.base_model}, {"model", v.model}};
}

void from_json(const ordered_json& j, HwModel& v)
{
    j.at("base_model").get_to(v.base_model);
    j.at("model").get_to(v.model);
}

void to_json(ordered_json& j, const HwFeederConfig& v)
{
    j = ordered_json{
        {"id", v.id},
        {"slot_count", v.slot_count},
        {"type", magic_enum::enum_name(v.type)},
        {"model", v.model},
        {"features", v.features}
    };
}

void from_json(const ordered_json& j, HwFeederConfig& v)
{
    j.at("id").get_to(v.id);
    j.at("slot_count").get_to(v.slot_count);
    v.type = magic_enum::enum_cast<FeederType>(j.at("type").get<std::string>()).value();
    j.at("model").get_to(v.model);
    j.at("features").get_to(v.features);
}

void to_json(ordered_json& j, const MaterialConfig& v)
{
    j = ordered_json{{"features", v.features}};
}

void from_json(const ordered_json& j, MaterialConfig& v)
{
    j.at("features").get_to(v.features);
}

void to_json(ordered_json& j, const HwPrinterConfig& v)
{
    j = ordered_json{
        {"id", v.id},
        {"printer_id", v.printer_id},
        {"vendor_id", v.vendor_id},
        {"name", v.name},
        {"technology", magic_enum::enum_name(v.technology)},
        {"model", v.model},
        {"tool_count", v.tool_count},
        {"features", v.features},
        {"tools", v.tools},
        {"feeders", v.feeders},
        {"materials", v.materials},
    };
}

} // namespace Slic3r::Domain::Preset

namespace Slic3r::Biz::Config {

using Domain::PrinterTechnology;
using Domain::Preset::FeatureValue;
using Domain::Preset::FeatureValueMap;
using Domain::Preset::FeederType;
using Domain::Preset::HwFeederConfig;
using Domain::Preset::HwFeederConfigs;
using Domain::Preset::HwMaterialConfigs;
using Domain::Preset::HwModel;
using Domain::Preset::HwPrinterConfig;
using Domain::Preset::HwToolConfig;
using Domain::Preset::HwToolConfigs;
using Domain::Preset::MaterialConfig;

template<>
tl::expected<void, std::string> is_valid<HwPrinterConfig>(const nlohmann::ordered_json& json_value)
{
    for (const auto& key : std::vector<std::string>{
             "id",
             "printer_id",
             "vendor_id",
             "name",
             "technology",
             "model",
             "tool_count",
             "features",
             "tools",
             "feeders",
             "materials",
         }) {
        if (!json_value.contains(key)) {
            return tl::unexpected{"'" + key + "' not present!"};
        }
    }
    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<HwModel>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.contains("base_model")) {
        return tl::unexpected{"'base_model' not present!"};
    }
    if (!json_value.at("base_model").is_string()) {
        return tl::unexpected{"'base_model' must be string!"};
    }
    if (!json_value.contains("model")) {
        return tl::unexpected{"'model' not present!"};
    }
    if (!json_value.at("model").is_string()) {
        return tl::unexpected{"'model' must be string!"};
    }
    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<uint8_t>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.is_number_unsigned()) {
        return tl::unexpected{"Value must be an unsigned number!"};
    }
    if (json_value.get<std::size_t>() > std::numeric_limits<uint8_t>::max()) {
        return tl::unexpected{"Value does not fit to uint8_t!"};
    }
    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<FeatureValue>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.is_boolean() && !json_value.is_number() && !json_value.is_string()) {
        return tl::unexpected{"Feature must be boolean, number or string!"};
    }
    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<HwToolConfig>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.contains("id")) {
        return tl::unexpected{"'id' not present!"};
    }

    if(!json_value.at("id").is_string()) {
        return tl::unexpected{"'id' is not a string!"};
    }
    if (!json_value.contains("features")) {
        return tl::unexpected{"'features' are not present"};
    }
    const auto features_valid{is_valid_map<FeatureValueMap>(json_value.at("features"))};
    if (!features_valid) {
        return features_valid;
    }

    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<HwFeederConfig>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.contains("id")) {
        return tl::unexpected{"'id' not present!"};
    }

    if(!json_value.at("id").is_string()) {
        return tl::unexpected{"'id' is not a string!"};
    }

    if (!json_value.contains("type")) {
        return tl::unexpected{"'type' not present"};
    }
    if (!json_value.at("type").is_string()) {
        return tl::unexpected{"'type' is not a string!"};
    }

    const std::string enum_value{json_value.at("type").get<std::string>()};
    if (!magic_enum::enum_contains<FeederType>(enum_value)) {
        return tl::unexpected{"'" + enum_value + "' is not a valid enum value"};
    };

    if (!json_value.contains("model")) {
        return tl::unexpected{"'model' not present"};
    }
    const auto model_valid{is_valid<HwModel>(json_value.at("model"))};
    if (!model_valid) {
        return model_valid;
    }
    if (!json_value.contains("slot_count")) {
        return tl::unexpected{"'slot_count' not present"};
    }
    if (!json_value.at("slot_count").is_number_unsigned()) {
        return tl::unexpected{"'slot_count' is not an unsigned number"};
    }

    if (!json_value.contains("features")) {
        return tl::unexpected{"'features' are not present"};
    }
    const auto features_valid{is_valid_map<FeatureValueMap>(json_value.at("features"))};
    if (!features_valid) {
        return features_valid;
    }

    return tl::expected<void, std::string>{};
}

template<>
tl::expected<void, std::string> is_valid<MaterialConfig>(const nlohmann::ordered_json& json_value)
{
    if (!json_value.contains("features")) {
        return tl::unexpected{"'features' are not present"};
    }
    const auto features_valid{is_valid_map<FeatureValueMap>(json_value.at("features"))};
    if (!features_valid) {
        return features_valid;
    }

    return tl::expected<void, std::string>{};
}

tl::expected<HwPrinterConfig, std::string> load_hw_config(const ordered_json& json)
{
    HwPrinterConfig result;

    if (!is_valid<HwPrinterConfig>(json)) {
        return tl::unexpected{"Invalid config structure!"};
    }

    const auto id{parse<std::string>(json.at("id"))};
    if (!id) {
        return tl::unexpected{"Invalid id: " + id.error()};
    }
    result.id = id.value();

    const auto printer_id{parse<std::string>(json.at("printer_id"))};
    if (!printer_id) {
        return tl::unexpected{"Invalid printer_id: " + printer_id.error()};
    }
    result.printer_id = printer_id.value();

    const auto vendor_id{parse<std::string>(json.at("vendor_id"))};
    if (!vendor_id) {
        return tl::unexpected{"Invalid vendor_id: " + vendor_id.error()};
    }
    result.vendor_id = vendor_id.value();

    const auto name{parse<std::string>(json.at("name"))};
    if (!name) {
        return tl::unexpected{"Invalid name: " + name.error()};
    }
    result.name = name.value();

    const auto technology{parse<std::string>(json.at("technology"))};
    if (!technology) {
        return tl::unexpected{"Invalid technology: " + technology.error()};
    }
    const auto technology_enum{magic_enum::enum_cast<PrinterTechnology>(*technology)};
    if (!technology_enum) {
        return tl::unexpected{"Invalid technology enum: " + *technology};
    }
    result.technology = technology_enum.value();

    const auto model{parse<HwModel>(json.at("model"))};
    if (!model) {
        return tl::unexpected{"Invalid model: " + model.error()};
    }
    result.model = model.value();

    const auto tool_count{parse<uint8_t>(json.at("tool_count"))};
    if (!tool_count) {
        return tl::unexpected{"Invalid tool_count: " + tool_count.error()};
    }
    result.tool_count = tool_count.value();

    const auto features{parse<FeatureValueMap>(json.at("features"))};
    if (!features) {
        return tl::unexpected{"Invalid features: " + features.error()};
    }
    result.features = features.value();

    const auto tools{parse<HwToolConfigs>(json.at("tools"))};
    if (!tools) {
        return tl::unexpected{"Invalid tools: " + tools.error()};
    }
    result.tools = tools.value();

    const auto partial_feeders{parse<PartialyParsedFeederConfigs>(json.at("feeders"))};
    if (!partial_feeders) {
        return tl::unexpected{"Invalid feeders: " + partial_feeders.error()};
    }
    HwFeederConfigs feeders;
    for (const auto& [key, value] : *partial_feeders) {
        const auto address{from_string(key)};
        if (!address) {
            return tl::unexpected{"Address could not be parsed from '" + key + "': " + address.error()};
        }
        feeders.insert({*address, value});
    }
    result.feeders = feeders;

    const auto partial_materials{parse<PartialyParsedMaterialConfigs>(json.at("materials"))};
    if (!partial_materials) {
        return tl::unexpected{"Invalid materials: " + partial_feeders.error()};
    }
    HwMaterialConfigs materials;
    for (const auto& [key, value] : *partial_materials) {
        const auto address{from_string(key)};
        if (!address) {
            return tl::unexpected{"Address could not be parsed from '" + key + "': " + address.error()};
        }
        materials.insert({*address, value});
    }
    result.materials = materials;

    return result;
}

} // namespace Slic3r::Biz::Config
