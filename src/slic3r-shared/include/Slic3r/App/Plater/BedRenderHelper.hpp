#pragma once

#include <vector>
#include <libslic3r/Point.hpp>

namespace Slic3r::Domain {
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class BedRenderHelper
{
public:
    /**
     * @brief Load the bed texture and return it in raw format.
     *
     * @param bed The bed whose texture is required.
     * @param size The desired size of the texture, in pixels.
     *
     * @return bed texture in raw format.
     *
     * @note The filename of the texture is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<uint8_t> texture(const Domain::Bed& bed, size_t size);

    /**
     * @brief Return the geometry of the bed grid.
     *
     * @param bed The bed whose grid is required.
     *
     * @return the geometry of the bed grid as a std::vector of vertices, two vertices for each segment.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<Vec3f> plate_grid(const Domain::Bed& bed);
};

} // namespace Slic3r::App::Plater