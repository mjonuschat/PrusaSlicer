#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Preset/SelectedPreset.hpp"

#include <libslic3r/Utils.hpp>

namespace Slic3r::Domain {

std::vector<size_t> BedContainer::beds_indices() const
{
    std::vector<size_t> ret;
    ret.reserve(m_beds.size());
    for (const auto& b : m_beds) {
        ret.emplace_back(b->id().id);
    }
    return ret;
}

Bed& BedContainer::add_bed(
    const Vec2ds& contour,
    float max_print_height,
    const std::optional<Bed::Segments>& bed_segments,
    const std::string& model_filename,
    const std::string& texture_filename
)
{
    m_beds.emplace_back(
        std::make_unique<Bed>(Bed::from(contour, max_print_height, bed_segments, model_filename, texture_filename))
    );
    return *m_beds.back();
}

Bed& BedContainer::add_bed(const Preset::SelectedPreset& preset, const PresetBundle& preset_bundle)
{
    const auto& printer_preset = preset.printer.config_box();
    auto shape_item            = printer_preset.contains("bed_shape");
    std::vector<Vec2d> shape;
    if (shape_item.item != nullptr)
        shape = shape_item.item->value().get<std::vector<Vec2d>>();

    auto max_print_height_item = printer_preset.contains("max_print_height");
    double max_print_height    = 0.0;
    if (max_print_height_item.item != nullptr)
        max_print_height = max_print_height_item.item->value().get<double>();

    std::string assets_path = Slic3r::resources_dir()
        + "/presets/"
        + preset.hw_config.repo_id
        + "/"
        + preset.hw_config.vendor_id
        + "/assets/";
    std::string model_filename;
    if (!preset.bed_model().empty())
        model_filename = assets_path + preset.bed_model();
    std::string texture_filename;
    if (!preset.bed_texture().empty())
        texture_filename = assets_path + preset.bed_texture();

    auto custom_model_filename_item = printer_preset.contains("bed_custom_model");
    std::string custom_model_filename;
    if (custom_model_filename_item.item != nullptr)
        custom_model_filename = custom_model_filename_item.item->value().get<std::string>();

    auto custom_texture_filename_item = printer_preset.contains("bed_custom_texture");
    std::string custom_texture_filename;
    if (custom_texture_filename_item.item != nullptr)
        custom_texture_filename = custom_texture_filename_item.item->value().get<std::string>();

    // TODO uncoment once selecting a preset is possible!
    const bool is_single_tool_XL{
        true
        //preset.hw_config.model.model == "XL" && preset.hw_config.tool_count == 1
    };
    const std::optional<Bed::Segments> segments{
        is_single_tool_XL ? std::optional{Bed::Segments{4, 4}} : std::nullopt
    };
    return add_bed(
        shape,
        float(max_print_height),
        segments,
        custom_model_filename.empty() ? model_filename : custom_model_filename,
        custom_texture_filename.empty() ? texture_filename : custom_texture_filename
    );
}

Bed* BedContainer::bed(size_t idx)
{
    auto it = std::find_if(m_beds.begin(), m_beds.end(), [idx](std::unique_ptr<Bed>& b) { return b->id().id == idx; });
    return (it != m_beds.end()) ? it->get() : nullptr;
}

const Bed* BedContainer::bed(size_t idx) const
{
    auto it = std::find_if(m_beds.begin(), m_beds.end(), [idx](const std::unique_ptr<Bed>& b) { return b->id().id == idx; });
    return (it != m_beds.end()) ? it->get() : nullptr;
}

} // namespace Slic3r::Domain
