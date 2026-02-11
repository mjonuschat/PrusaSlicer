#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"

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

Bed& BedContainer::get_or_create_bed(const ConfigContainer& config_container, const std::string& resources_dir_path,
    SelectionId project_id, SelectionId config_container_id, std::function<Vec2ds(SelectionId, SelectionId)> system_preset_bed_shape_getter)
{
    const auto& preset = config_container.selected_preset();
    const auto& config_box = preset.printer.config_box();

    auto item = config_box.find("bed_shape");
    Vec2ds bed_shape = (item.item != nullptr) ? item.item->value().get<Vec2ds>() : Vec2ds();

    bool use_model_and_texture = system_preset_bed_shape_getter != nullptr &&
                                 project_id != INVALID_ID &&
                                 config_container_id != INVALID_ID;
    if (use_model_and_texture) {
        // if the bed shape is different between selected preset and system preset, we do not use the model/texture from assets
        Vec2ds sys_bed_shape = system_preset_bed_shape_getter(project_id, config_container_id);
        use_model_and_texture = bed_shape == sys_bed_shape;
    }

    std::string model_filename;
    std::string texture_filename;
    if (use_model_and_texture) {
        std::string bed_model_filename = preset.bed_model();
        std::string bed_texture_filename = preset.bed_texture();
        std::string assets_path = resources_dir_path + "/presets/" + preset.hw_config.repo_id + "/" + preset.hw_config.vendor_id + "/assets/";
        if (!bed_model_filename.empty())
            model_filename = assets_path + bed_model_filename;
        if (!bed_texture_filename.empty())
            texture_filename = assets_path + bed_texture_filename;
    }

    item = config_box.find("max_print_height");
    float bed_max_print_height = (item.item != nullptr) ? float(item.item->value().get<double>()) : 0.0f;

    item = config_box.find("bed_custom_model");
    std::string custom_bed_model_filename = (item.item != nullptr) ? item.item->value().get<std::string>() : std::string();

    item = config_box.find("bed_custom_texture");
    std::string custom_bed_texture_filename = (item.item != nullptr) ? item.item->value().get<std::string>() : std::string();

    Bed::Segments bed_segments{1, 1};
    if (auto bed_segments_x{
            Preset::get_feature<int>(preset.hw_config.features, "bed_segments_x")
    }) {
        bed_segments.x_count = *bed_segments_x;
    }
    if (auto bed_segments_y{
        Preset::get_feature<int>(preset.hw_config.features, "bed_segments_y")
    }) {
        bed_segments.y_count = *bed_segments_y;
    }
    const std::optional<Bed::Segments> segments{
        bed_segments != Bed::Segments{1, 1} ? std::optional{bed_segments} : std::nullopt
    };

    Vec2d auxiliary_travel_anchor{-1, -1};
    if (auto auxiliary_travel_anchor_x{
        Preset::get_feature<double>(preset.hw_config.features, "auxiliary_travel_anchor_x")
    }) {
        auxiliary_travel_anchor.x() = *auxiliary_travel_anchor_x;
    }
    if (auto auxiliary_travel_anchor_y{
        Preset::get_feature<double>(preset.hw_config.features, "auxiliary_travel_anchor_y")
    }) {
        auxiliary_travel_anchor.y() = *auxiliary_travel_anchor_y;
    }
    const std::optional<Vec2d> travel_anchor{
        (auxiliary_travel_anchor.array() >= 0).all() ? std::optional{auxiliary_travel_anchor} :
                                                       std::nullopt
    };

    auto bed = Bed::from(
        bed_shape,
        bed_max_print_height,
        segments,
        travel_anchor,
        custom_bed_model_filename.empty() ? model_filename : custom_bed_model_filename,
        custom_bed_texture_filename.empty() ? texture_filename : custom_bed_texture_filename
    );

    auto it = std::ranges::find_if(m_beds, [&](const auto& present_bed) { return present_bed->matches(bed); });
    if (it != m_beds.end())
        return **it;

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
