#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Slic3r/Domain/Bed.hpp"
#include "libslic3r/Preset.hpp"

namespace Slic3r {
class DynamicPrintConfig;
class PresetBundle;
} // namespace Slic3r

namespace Slic3r::Domain {

class Bed;
class BedInstance;

class BedContainer
{
public:
    [[nodiscard]] Bed& add_bed(
        const Pointfs& contour,
        float max_print_height,
        const std::string& model_filename,
        const std::string& texture_filename
    );

    Bed& add_bed(const Preset& selected_preset, const PresetBundle& preset_bundle);

    size_t beds_count() const { return m_beds.size(); }
    std::vector<size_t> beds_indices() const;

    [[nodiscard]] Bed* bed(size_t idx);
    [[nodiscard]] const Bed* bed(size_t idx) const;

    using BedList = std::vector<std::unique_ptr<Bed>>;
    [[nodiscard]] const BedList& beds() const { return m_beds; }

    void reset() { m_beds.clear(); }

private:
    BedList m_beds;
};

} // namespace Slic3r::Domain
