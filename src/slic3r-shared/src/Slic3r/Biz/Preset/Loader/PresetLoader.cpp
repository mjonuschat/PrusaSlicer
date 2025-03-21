#include "Slic3r/Biz/Preset/Loader/PresetLoader.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"

ENUM_DESC(Slic3r::Domain::Preset::PresetKind,
    ("printer", FdmPrinter),
    ("print", FdmPrint),
    ("tool_print", FdmToolPrint),
    ("filament", FdmMaterial)
);

#define PRESET_VARIANT_FIELDS                               \
    FIELD_DESC(id, FIELD_DEFAULT, {}, FIELD_DEFAULT),       \
    FIELD_DESC_SIMPLE(name),                                \
    FIELD_DESC(inherits, FIELD_DEFAULT, {}, FIELD_DEFAULT), \
    FIELD_DESC(values, FIELD_DEFAULT, {}, FIELD_DEFAULT),   \
    FIELD_DESC_SIMPLE(condition),                           \
    FIELD_DESC(variants, FIELD_DEFAULT, {}, FIELD_DEFAULT)

STRUCT_DESC(Slic3r::Domain::Preset::PresetNode,
    PRESET_VARIANT_FIELDS
);

STRUCT_DESC(Slic3r::Domain::Preset::RootPresetNode,
    FIELD_DESC_SIMPLE(kind),
    PRESET_VARIANT_FIELDS
);

#undef PRESET_VARIANT_FIELDS

namespace Slic3r::Biz::Preset::Loader {

void PresetLoader::load(const std::string& file_name)
{
    Yaml::parse_all_documents_in_file(file_name.c_str(), [this](const auto& doc) {
        auto preset = Yaml::parse_struct<RootPresetNode>(doc);
        m_presets[preset.kind].emplace_back(std::move(preset));
    });
}

} // namespace Slic3r::Biz::Preset::Loader
