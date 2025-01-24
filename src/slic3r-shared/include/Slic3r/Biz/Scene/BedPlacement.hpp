#pragma once

#include <libslic3r/Point.hpp>

namespace Slic3r::Domain {
class BedContainer;
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Scene {

class BedPlacement
{
public:
    /**
     * @brief Perform the layout of bed instances in the scene.
     *
     * @param project The project containing the bed instances to layout.
     * @param gap The space to leave between two adjacent instances.
     */
    void layout(Domain::Project& project, const Vec2d& gap);

private:
//    Vec2d m_max_size{ Vec2d::Zero() };
};

} // namespace Slic3r::Biz::Scene
