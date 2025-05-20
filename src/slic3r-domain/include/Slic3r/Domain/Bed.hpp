#pragma once

#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

namespace Slic3r::Domain {

class Bed : public ObjectBase
{
public:
    [[nodiscard]] static Bed from(const Vec2ds& contour, float max_print_height,
        const std::string& model_filename, const std::string& texture_filename);

    [[nodiscard]] const Vec2d& center() const { return m_center; }
    [[nodiscard]] const Vec2d& offset() const { return m_offset; }
    [[nodiscard]] const Vec2ds& contour() const { return m_contour; }
    [[nodiscard]] const Vec2d& contour_aabb_extent() const { return m_contour_aabb_extent; }
    [[nodiscard]] float max_print_height() const { return m_max_print_height; }

    [[nodiscard]] const std::string& model_filename() const { return m_model_filename; }
    [[nodiscard]] const std::string& texture_filename() const { return m_texture_filename; }
    [[nodiscard]] bool contains(const Vec2d& bed_inst_position, const BoundingBox2d& object_bb) const;


private:
    Vec2d m_center{ Vec2d::Zero() };
    Vec2d m_offset{ Vec2d::Zero() };
    Vec2ds m_contour;
    Vec2d m_contour_aabb_extent{ Vec2d::Zero() };
    float m_max_print_height{ 0.0f };
    std::string m_model_filename;
    std::string m_texture_filename;
};

} // namespace Slic3r::Domain
