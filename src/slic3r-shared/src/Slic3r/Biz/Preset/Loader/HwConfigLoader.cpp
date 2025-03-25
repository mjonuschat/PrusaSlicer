#include "Slic3r/Biz/Preset/Loader/HwConfigLoader.hpp"

#include "Yaml.hpp"
#include "Yaml.hpp"

using namespace Slic3r::Domain::Preset;

STRUCT_DESC(HwModel,
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(base_model)
);

STRUCT_DESC(HwToolConfig,
    FIELD_DESC_SIMPLE(features),
    FIELD_DESC_SIMPLE(nozzle_diameter)
);

STRUCT_DESC_SIMPLE(HwFeederConfig,
    id, type, model, slot_count, features
);
STRUCT_DESC_SIMPLE(MaterialConfig,
    features
);
STRUCT_DESC_SIMPLE(HwPrinterConfig,
    id, technology, model, tool_count, features, tools, feeders, materials
);

STRUCT_DESC(FeatureDef,
    FIELD_DESC_SIMPLE(name),
    FIELD_DESC(allowed_values, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC_SIMPLE(default_value),
    FIELD_DESC(user_editable, FIELD_DEFAULT, true, FIELD_DEFAULT)
);

STRUCT_DESC(HwPrinterConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC_SIMPLE(features)
);

STRUCT_DESC(HwToolConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(condition),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC_SIMPLE(features)
);

STRUCT_DESC(HwFeederConfigDef,
    FIELD_DESC_SIMPLE(id),
    FIELD_DESC_SIMPLE(model),
    FIELD_DESC_SIMPLE(technology),
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),
    FIELD_DESC_SIMPLE(condition)
);


namespace Slic3r::Biz::Preset::Loader {

HwConfigLoader::HwConfigLoader()
    : m_result{
        {Domain::PrinterTechnology::FFF, {Domain::PrinterTechnology::FFF}},
        {Domain::PrinterTechnology::SLA, {Domain::PrinterTechnology::SLA}}
    }
{}

Domain::Preset::HwDefs& HwConfigLoader::load(const std::string & filename)
{
    Yaml::parse_all_documents_in_file(filename.c_str(), [this](const auto& doc) {
        Yaml::parse_structs_by_discriminant(
            fy_document_root(doc.get()),
            "kind",
            std::make_tuple(
                "printer",
                std::function{[this](Domain::Preset::HwPrinterConfigDef&& p) {
                    m_result[p.technology].printers.emplace(p.id, p);
                }}
            ),
            std::make_tuple(
                "tool",
                std::function{[this](Domain::Preset::HwToolConfigDef&& p) {
                    m_result[p.technology].tools.emplace(p.id, p);
                }}
            ),
            std::make_tuple(
                "feeder",
                std::function{[this](Domain::Preset::HwFeederConfigDef&& p) {
                    m_result[p.technology].feeders.emplace(p.id, p);
                }}
            )
        );

    });

    return m_result;
}

}