#pragma once

#include "libslic3r/ObjectID.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Geometry.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

namespace Slic3r::Domain {

class Bed : public ObjectBase
{
public:
    Bed() = default;
    Bed(Bed&&) = default;
    Bed& operator=(Bed&&) = default;

    ~Bed() { clear_instances(); }

    [[nodiscard]] static Bed from(const Pointfs& contour, float max_print_height,
        const std::string& model_filename, const std::string& texture_filename);

    [[nodiscard]] const Vec2d& center() const { return m_center; }
    [[nodiscard]] const Pointfs& contour() const { return m_contour; }
    [[nodiscard]] float max_print_height() const { return m_max_print_height; }
    [[nodiscard]] const Vec2d& outer_size() const { return m_outer_size; }

    [[nodiscard]] const std::string& model_filename() const { return m_model_filename; }
    [[nodiscard]] const std::string& texture_filename() const { return m_texture_filename; }

    [[nodiscard]] BedInstance& add_instance();
    [[nodiscard]] BedInstance& add_instance(const Geometry::Transformation& trafo);
    void remove_instance(size_t idx);
    void clear_instances();

    [[nodiscard]] BedInstance* instance(size_t idx);
    [[nodiscard]] const BedInstance* instance(size_t idx) const;

    using BedInstances = std::vector<std::unique_ptr<BedInstance>>;
    [[nodiscard]] BedInstances& instances() { return m_instances; }
    [[nodiscard]] const BedInstances& instances() const { return m_instances; }

private:
    Pointfs m_contour;
    Vec2d m_center{ Vec2d::Zero() };
    Vec2d m_outer_size{ Vec2d::Zero() };
    float m_max_print_height{ 0.0f };
    std::string m_model_filename;
    std::string m_texture_filename;
    BedInstances m_instances;
};

} // namespace Slic3r::Domain
