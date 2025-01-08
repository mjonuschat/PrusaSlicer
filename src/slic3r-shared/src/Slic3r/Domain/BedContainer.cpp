#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/PresetBundle.hpp>

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
    const Pointfs& contour,
    float max_print_height,
    const std::string& model_filename,
    const std::string& texture_filename
)
{
    m_beds.emplace_back(
        std::make_unique<Bed>(Bed::from(contour, max_print_height, model_filename, texture_filename))
    );
    return *m_beds.back();
}

Bed& BedContainer::add_bed(const Preset& selected_preset, const PresetBundle& preset_bundle)
{
    std::string model_filename;
    std::string texture_filename;

    const Slic3r::Preset* curr_preset = &selected_preset;
    while (curr_preset != nullptr) {
        if (curr_preset->config.has("bed_shape")) {
            model_filename   = PresetUtils::system_printer_bed_model(*curr_preset);
            texture_filename = PresetUtils::system_printer_bed_texture(*curr_preset);
            if (!model_filename.empty() && !texture_filename.empty())
                break;
        }
        curr_preset = preset_bundle.printers.get_preset_parent(*curr_preset);
    }

    std::string custom_model_filename   = selected_preset.config.option<ConfigOptionString>("bed_custom_model")->value;
    std::string custom_texture_filename = selected_preset.config.option<ConfigOptionString>("bed_custom_texture")->value;

    return add_bed(
        selected_preset.config.option<ConfigOptionPoints>("bed_shape")->values,
        selected_preset.config.option<ConfigOptionFloat>("max_print_height")->value,
        custom_model_filename.empty() ? model_filename : custom_model_filename,
        custom_texture_filename.empty() ? texture_filename : custom_texture_filename);
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

BedInstance* BedContainer::bed_instance(size_t bed_idx, size_t instance_idx)
{
    Bed* b = bed(bed_idx);
    if (b != nullptr) {
        Bed::BedInstances& instances = b->instances();
        auto it = std::find_if(instances.begin(), instances.end(), [instance_idx](const auto& i) { return i->id().id == instance_idx; });
        if (it != instances.end())
            return it->get();
    }
    return nullptr;
}

const BedInstance* BedContainer::bed_instance(size_t bed_idx, size_t instance_idx) const
{
    const Bed* b = bed(bed_idx);
    if (b != nullptr) {
        const Bed::BedInstances& instances = b->instances();
        auto it = std::find_if(instances.begin(), instances.end(), [instance_idx](const auto& i) { return i->id().id == instance_idx; });
        if (it != instances.end())
            return it->get();
    }
    return nullptr;
}

} // namespace Slic3r::Domain
