#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Slic3r/Domain/Bed.hpp"

namespace Slic3r::Domain::Preset {
struct SelectedPreset;
} // namespace Slic3r::Domain::Preset

namespace Slic3r::Domain {

class Bed;
struct BedInstance;

class BedContainer
{
public:
    [[nodiscard]] Bed& get_or_create_bed(const Preset::SelectedPreset& preset, const std::string& resources_dir_path);

    void remove(const Bed* bed);

    size_t beds_count() const { return m_beds.size(); }
    std::vector<size_t> beds_indices() const;

    [[nodiscard]] Bed* bed(size_t idx);
    [[nodiscard]] const Bed* bed(size_t idx) const;

    using BedList = std::vector<std::unique_ptr<Bed>>;
    [[nodiscard]] const BedList& beds() const { return m_beds; }
    [[nodiscard]] BedList& beds() { return m_beds; }

    void reset() { m_beds.clear(); }

private:
    [[nodiscard]] Bed& add_bed(
        const Vec2ds& contour,
        float max_print_height,
        const std::optional<Bed::Segments>& bed_segments,
        const std::string& model_filename,
        const std::string& texture_filename
    );

    BedList m_beds;
};

} // namespace Slic3r::Domain
