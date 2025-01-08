#pragma once

#include <vector>
#include <libslic3r/Point.hpp>
#include "Slic3r/App/Render/TextureManager.hpp"

namespace Slic3r::Domain {
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class BedRenderHelper
{
public:
    /**
     * @brief Load the bed texture and return it.
     *
     * @param bed The bed whose texture is required.
     * @param manager The TextureManager instance to create texture within
     *
     * @return bed texture instance.
     *
     * @note The filename of the texture is specified into the bed, see Slic3r::Domain::Bed definition.
     * @note The texture size is equal to half of the max texture size supported by the graphic card.
     */
    [[nodiscard]] static Render::Texture* texture(const Domain::Bed& bed, Render::TextureManager& manager);

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