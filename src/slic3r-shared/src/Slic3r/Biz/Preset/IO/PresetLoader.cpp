#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"
#include "Slic3r/Biz/Yaml/YamlSlic3rTypes.hpp"

#include <boost/filesystem/directory.hpp>

#include "oneapi/tbb/parallel_for.h"

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

ENUM_DESC(Slic3r::Domain::Preset::ConditionMatchMode,
    ("first_match", FirstMatch),
    ("all_matches", AllMatches)
)

#define PRESET_VARIANT_FIELDS                                                   \
    FIELD_DESC(id, FIELD_DEFAULT, {}, FIELD_DEFAULT),                           \
    FIELD_DESC_SIMPLE(name),                                                    \
    FIELD_DESC(inherits, FIELD_DEFAULT, {}, FIELD_DEFAULT),                     \
    FIELD_DESC(unconditional_inherits, FIELD_DEFAULT, {}, FIELD_DEFAULT),       \
    FIELD_DESC(values, FIELD_DEFAULT, {}, FIELD_DEFAULT),                       \
    FIELD_DESC(features, FIELD_DEFAULT, {}, FIELD_DEFAULT),                     \
    FIELD_DESC_SIMPLE(condition),                                               \
    FIELD_DESC_SIMPLE(match_mode),                                              \
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

void PresetLoader::load(const std::string& file_name, std::mutex& mutex)
{
    Yaml::parse_all_documents_in_file(file_name.c_str(), [this, &mutex](const auto& doc) {
        auto preset = Yaml::parse_struct_unwrap<RootPresetNode>(doc);
        std::lock_guard guard(mutex);
        m_presets[preset.kind].emplace_back(std::move(preset));
    });
}

void PresetLoader::load_from_string(std::string_view source)
{
    Yaml::parse_all_documents_in_string(source, [this](const auto& doc) {
        auto preset = Yaml::parse_struct_unwrap<RootPresetNode>(doc);
        m_presets[preset.kind].emplace_back(std::move(preset));
    });
}

void PresetLoader::load_dir(const std::string& dir_path)
{
    // Collect paths to files to load from.
    std::vector<boost::filesystem::path> paths;
    for (const auto& entry : boost::filesystem::directory_iterator{ dir_path }) {
        boost::filesystem::path path = entry.path();
        if (is_regular_file(entry) && path.has_extension() && path.extension() == ".yaml" && path.filename() != "vendor.yaml")
            paths.emplace_back(path);
    }

    // The presets are loaded in parallel and added into m_presets, which is
    // guarded by this mutex.
    std::mutex mutex;
    tbb::parallel_for(tbb::blocked_range<size_t>(0, paths.size()),
        [this, &mutex, &paths](const tbb::blocked_range<size_t> &range) {
            for (size_t i = range.begin(); i < range.end(); ++i)
                load(paths[i].string(), mutex);
        }
    );
}

} // namespace Slic3r::Biz::Preset::Loader
