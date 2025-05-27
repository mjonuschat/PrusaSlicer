#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"
#include "Slic3r/Biz/Preset/IO/YamlSlic3rTypes.hpp"
#include "Slic3r/Log.hpp"

#include <fstream>
#include <ranges>
#include <set>

#include "Yaml.hpp"
#include "libslic3r/Utils/DirectoriesUtils.hpp"

#include <charconv>
#include <nlohmann/json.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <fmt/ranges.h>


using namespace Slic3r::Domain::Preset;
namespace fs = boost::filesystem;
using json = nlohmann::json;


NLOHMANN_JSON_NAMESPACE_BEGIN

template<>
struct adl_serializer<FeatureValue>
{

    static void to_json(json& j, const FeatureValue& v)
    {
        std::visit([&j](auto&& arg) { j = arg; }, v);
    }

    static void from_json(const json& j, FeatureValue& v)
    {
        if (j.is_boolean())
            v = j.get<bool>();
        else if (j.is_number())
            v = j.get<float>();
        else if (j.is_string())
            v = j.get<std::string>();
    }
};

template<>
struct adl_serializer<HwToolConfig>
{
    static void to_json(json& j, const HwToolConfig& v)
    {
        j = json{ {"id", v.id}, {"features", v.features}};
    }

    static void from_json(const json& j, HwToolConfig& v)
    {
        j.at("id").get_to(v.id);
        j.at("features").get_to(v.features);
    }
};

template<>
struct adl_serializer<Address>
{
    static void to_json(json& j, const Address& v)
    {
        j = fmt::to_string(fmt::join(v | std::views::transform([](auto slot) { return int(slot); }), "."));
    }

    static void from_json(const json& j, Address& v)
    {
        v.clear();
        for (const auto& slot : std::views::split(j.get<std::string>(), ".")){
            int slot_v;
            auto result = std::from_chars(std::to_address(slot.begin()), std::to_address(slot.end()), slot_v, 10);
            ASSERT(result.ec == std::errc(), std::make_tuple(v, slot));
            v.push_back(slot_v);
        }
    }
};

template<>
struct adl_serializer<HwModel>
{
    static void to_json(json& j, const HwModel& v)
    {
        j = json{{"base_model", v.base_model}, {"model", v.model}};
    }

    static void from_json(const json& j, HwModel& v)
    {
        j.at("base_model").get_to(v.base_model);
        j.at("model").get_to(v.model);
    }
};

template <>
struct adl_serializer<HwFeederConfig>
{
    static void to_json(json& j, const HwFeederConfig& v)
    {
        j = json{ {"id", v.id}, {"slot_count", v.slot_count}, {"type", v.type}, {"model", v.model}, {"features", v.features}};
    }

    static void from_json(const json& j, HwFeederConfig& v)
    {
        j.at("id").get_to(v.id);
        j.at("slot_count").get_to(v.slot_count);
        j.at("type").get_to(v.type);
        j.at("model").get_to(v.model);
        j.at("features").get_to(v.features);

    }
};

template<>
struct adl_serializer<HwPrinterConfig>
{
    static void to_json(json& j, const HwPrinterConfig& v)
    {
        j = json{ {"id", v.id}, {"printer_id", v.printer_id}, {"vendor_id", v.vendor_id}, {"features", v.features}};
    }

    static void from_json(const json& j, HwPrinterConfig& v)
    {
        j.at("id").get_to(v.id);
        j.at("printer_id").get_to(v.printer_id);
        j.at("vendor_id").get_to(v.vendor_id);
        j.at("features").get_to(v.features);

        v.tools.clear();
        auto tools = j.at("tools");
        for (const auto& t : tools) {
            HwToolConfig tc;
            Address addr;
            t.get_to(tc);
            t["address"].get_to(addr);
            v.tools.emplace_back(t);
        }

        auto feeders = j.at("feeders");
        for (const auto& f : feeders) {
            HwFeederConfig fc;
            Address addr;
            f.get_to(fc);
            f["address"].get_to(addr);
            v.feeders.emplace(addr, fc);
        }
    }
};
NLOHMANN_JSON_NAMESPACE_END

STRUCT_DESC(HwModel,
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(base_model)
);

STRUCT_DESC(HwToolConfig,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(features)
);

STRUCT_DESC_SIMPLE(HwFeederConfig,
    type, model, slot_count, features
);
STRUCT_DESC_SIMPLE(MaterialConfig,
    features
);
STRUCT_DESC_SIMPLE(HwPrinterConfig,
    id, printer_id, vendor_id, name, technology, model, tool_count, features, tools, feeders, materials
);

STRUCT_DESC(FeatureDef,
    FIELD_DESC(allowed_values, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(default_value, "default", FIELD_DEFAULT, FIELD_DEFAULT),
    FIELD_DESC(user_editable, FIELD_DEFAULT, true, FIELD_DEFAULT)
);

STRUCT_DESC(HwPrinterConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(tool_count, FIELD_DEFAULT, 1, FIELD_DEFAULT)
);

STRUCT_DESC(HwToolConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(condition),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT)
);

STRUCT_DESC(HwFeederConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC(type, FIELD_DEFAULT, FeederType::MMU, FIELD_DEFAULT),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC_SIMPLE(slot_count),
    FIELD_DESC_SIMPLE(condition)
);

STRUCT_DESC(HwSheetConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(type),
    FIELD_DESC_SIMPLE(condition),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT)
)

STRUCT_DESC(HwToolConfigTemplate,
    FIELD_DESC(id, "tool", FIELD_DEFAULT, FIELD_DEFAULT),
    FIELD_DESC(features,FIELD_DEFAULT, {}, FIELD_DEFAULT)
);

STRUCT_DESC(HwFeederConfigTemplate,
    FIELD_DESC(id, "feeder", FIELD_DEFAULT, FIELD_DEFAULT),
    FIELD_DESC(features,FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC_SIMPLE(address)
);

STRUCT_DESC(HwPrinterConfigTemplate,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(printer),
    FIELD_DESC_SIMPLE(sheet),
    FIELD_DESC_SIMPLE(tool_count),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(tools, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(feeders, FIELD_DEFAULT, {}, FIELD_DEFAULT)
);

STRUCT_DESC(VendorFeatures,
    FIELD_DESC(printer, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(tool, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(feeder, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC(sheet, FIELD_DEFAULT, {}, FIELD_DEFAULT)
);

STRUCT_DESC(VendorInfo,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC_SIMPLE(version),
    FIELD_DESC_SIMPLE(features)
);

namespace Slic3r::Biz::Preset::IO {

HwConfigLoader::HwConfigLoader()
{ init_result(); }

void HwConfigLoader::init_result()
{
    m_result = {
        .defs = {
            {Domain::PrinterTechnology::FFF, {Domain::PrinterTechnology::FFF}},
            {Domain::PrinterTechnology::SLA, {Domain::PrinterTechnology::SLA}}
        }
    };
}

Domain::Preset::VendorData& HwConfigLoader::load(const std::string& filename)
{
    Yaml::parse_all_documents_in_file(filename.c_str(), [this](const auto& doc) {
        Yaml::parse_structs_by_discriminant(
            doc.root(),
            "kind",
            std::make_tuple(
                "printer",
                std::function{[this](Domain::Preset::HwPrinterConfigDef&& p) {
                    m_result.defs[p.technology].printers.emplace(p.id, p);
                }}
            ),
            std::make_tuple(
                "tool",
                std::function{[this](Domain::Preset::HwToolConfigDef&& t) {
                    m_result.defs[t.technology].tools.emplace(t.id, t);
                }}
            ),
            std::make_tuple(
                "feeder",
                std::function{[this](Domain::Preset::HwFeederConfigDef&& f) {
                    m_result.defs[f.technology].feeders.emplace(f.id, f);
                }}
            ),
            std::make_tuple(
                "sheet",
                std::function{[this](Domain::Preset::HwSheetConfigDef&& s) {
                    m_result.defs[Domain::PrinterTechnology::FFF].sheets.emplace(s.id, s);
                }}
            ),
            std::make_tuple(
                "printer_config",
                std::function{[this](HwPrinterConfigTemplate&& p) {
                    m_result.printer_configs.emplace_back(p);
                }}
            ),
            std::make_tuple(
                "vendor",
                std::function{[this](VendorInfo&& v) {
                    m_result.info = v;
                }}
            )
        );
    });

    return m_result;
}


Domain::Preset::HwPrinterConfigs load_vendor_user_configs(const std::string& dir_path, const Domain::Preset::VendorData& vendor_data)
{
    HwPrinterConfigs ret;

    if (!fs::exists(dir_path)) {
        SPDLOG_DEBUG("Skipping loading vendor configs from directory '{}' as it not exists", dir_path);
        return ret;
    }

    for (const auto& entry : fs::directory_iterator{dir_path}) {
        if (!entry.is_regular_file())
            continue;

        auto p = entry.path();
        if (p.extension() != ".json")
            continue;

        HwPrinterConfig config;

        try {
            std::ifstream f(p.string());
            json j = json::parse(f);
            f.close();
            j.get_to(config);
        }
        catch (json::parse_error& e){
            SPDLOG_INFO("Parsing HwPritnerConfig {} failed: {}", p.string(), e.what());
        }


        fill_missing_features_with_default(config, vendor_data);
    }

    return ret;
}

void save_vendor_user_configs(const Domain::Preset::HwPrinterConfigs& configs, const std::string& dir_path, const Domain::Preset::VendorData& vendor_data)
{
    fs::path dir{dir_path};
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    size_t idx = 0;

    for (const auto& base_config : configs) {
        auto config = remove_features_with_default(base_config, vendor_data);

        json j;
        nlohmann::adl_serializer<HwPrinterConfig>::to_json(j, config);

        std::string name = config.name;
        if (name.empty())
            name = fmt::format("printer{:03d}", idx++);
        std::ofstream out{(dir / (name + ".json")).string()};
        out << j;
        out.close();
    }
}

}