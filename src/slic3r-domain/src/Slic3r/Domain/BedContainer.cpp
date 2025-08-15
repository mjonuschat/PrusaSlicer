#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Preset/SelectedPreset.hpp"

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

Bed& BedContainer::get_or_create_bed(const Preset::SelectedPreset& preset, const std::string& resources_dir_path)
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

    std::string assets_path = resources_dir_path
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

    const bool is_single_tool_XL{
        preset.hw_config.model.model == "XL" && preset.hw_config.tool_count == 1
    };
    const std::optional<Bed::Segments> segments{
        is_single_tool_XL ? std::optional{Bed::Segments{4, 4}} : std::nullopt
    };

    auto bed{Bed::from(
        shape,
        static_cast<float>(max_print_height),
        segments,
        custom_model_filename.empty() ? model_filename : custom_model_filename,
        custom_texture_filename.empty() ? texture_filename : custom_texture_filename
    )};

    const auto it{
        std::ranges::find_if(m_beds, [&](const auto& present_bed) { return *present_bed == bed; })
    };
    if (it != m_beds.end()) {
        return **it;
    }

    m_beds.push_back(std::make_unique<Bed>(std::move(bed)));
    return *m_beds.back();
}

void BedContainer::remove(const Bed* bed)
{
    std::erase_if(m_beds, [&](const auto& bed_in_beds) { return bed_in_beds.get() == bed; });
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
