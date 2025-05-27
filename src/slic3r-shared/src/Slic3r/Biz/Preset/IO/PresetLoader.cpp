#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Preset/IO/Yaml.hpp"
#include "Slic3r/Biz/Preset/IO/YamlSlic3rTypes.hpp"

#include <boost/filesystem/directory.hpp>

ENUM_DESC(Slic3r::Domain::Preset::PresetKind,
    ("printer", FdmPrinter),
    ("print", FdmPrint),
    ("tool_print", FdmToolPrint),
    ("filament", FdmMaterial),
    ("sla_printer", SlaPrinter),
    ("sla_print", SlaPrint),
    ("sla_tool_print", SlaToolPrint),
    ("sla_material", SlaMaterial)
);

#define PRESET_VARIANT_FIELDS                                                   \
    FIELD_DESC(id, FIELD_DEFAULT, {}, FIELD_DEFAULT),                           \
    FIELD_DESC_SIMPLE(name),                                                    \
    FIELD_DESC(inherits, FIELD_DEFAULT, {}, FIELD_DEFAULT),                     \
    FIELD_DESC(unconditional_inherits, FIELD_DEFAULT, {}, FIELD_DEFAULT),       \
    FIELD_DESC(values, FIELD_DEFAULT, {}, FIELD_DEFAULT),                       \
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),                     \
    FIELD_DESC_SIMPLE(condition),                                               \
    FIELD_DESC(variants, FIELD_DEFAULT, {}, FIELD_DEFAULT),                     \
    FIELD_DESC(source_location, FIELD_NAME_SELF, FIELD_DEFAULT, FIELD_DEFAULT)

STRUCT_DESC(Slic3r::Domain::Preset::PresetNode,
    PRESET_VARIANT_FIELDS
);

STRUCT_DESC(Slic3r::Domain::Preset::RootPresetNode,
    FIELD_DESC_SIMPLE(kind),
    PRESET_VARIANT_FIELDS
);

#undef PRESET_VARIANT_FIELDS

namespace Slic3r::Biz::Preset::IO {

void PresetLoader::load(const std::string& file_name)
{
    Yaml::parse_all_documents_in_file(file_name.c_str(), [this](const auto& doc) {
        auto preset = Yaml::parse_struct<RootPresetNode>(doc);
        m_presets[preset.kind].emplace_back(std::move(preset));
    });
}

void PresetLoader::load_from_string(std::string_view source)
{
    Yaml::parse_all_documents_in_string(source, [this](const auto& doc) {
        auto preset = Yaml::parse_struct<RootPresetNode>(doc);
        m_presets[preset.kind].emplace_back(std::move(preset));
    });
}

void PresetLoader::load_dir(const std::string& dir_path)
{
    for (const auto& entry : boost::filesystem::directory_iterator{dir_path}) {
        auto path = entry.path();
        if (!is_regular_file(entry) || !path.has_extension() || path.extension() != ".yaml" || path.filename() == "vendor.yaml")
            continue;
        load(path.string());
    }
}

} // namespace Slic3r::Biz::Preset::Loader
