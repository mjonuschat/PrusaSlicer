#include "Slic3r/Biz/Preset/Loader/HwConfigLoader.hpp"
#include "Slic3r/Biz/Preset/Loader/YamlSlic3rTypes.hpp"

#include <set>

#include "Yaml.hpp"

using namespace Slic3r::Domain::Preset;

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
    id, vendor_id, name, technology, model, tool_count, features, tools, feeders, materials
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

namespace Slic3r::Biz::Preset::Loader {

HwConfigLoader::HwConfigLoader()
    : m_result{
        .defs = {
            {Domain::PrinterTechnology::FFF, {Domain::PrinterTechnology::FFF}},
            {Domain::PrinterTechnology::SLA, {Domain::PrinterTechnology::SLA}}
        }
    }
{}

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

    std::set<std::string> referenced_printer_ids = {};
    for (const auto& t : m_result.printer_configs) {
        referenced_printer_ids.insert(t.id);
    }

    for (const auto& [pt, defs] : m_result.defs) {
        for (const auto& [id, p] : defs.printers) {
            if (referenced_printer_ids.contains(id))
                continue;

            HwPrinterConfigTemplate t = {.id = id, .name = p.name, .tool_count = p.tool_count, .tools = {}};
        }
    }




    return m_result;
}

}