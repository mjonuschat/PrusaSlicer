#pragma once

#include <libslic3r/TriangleMesh.hpp>

namespace Slic3r::Domain {
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Plater {

class BedGeometry
{
public:
    /**
     * @brief Load the bed model and return its geometry.
     * 
     * @param bed The bed whose model is required.
     *
     * @return the geometry of the bed model as TriangleMesh.
     * 
     * @note The filename of the model is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static TriangleMesh model(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed plate.
     *
     * @param bed The bed whose plate is required.
     *
     * @return the geometry of the bed plate as a std::vector of pairs vertex-uvs, three pairs for each triangle.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<std::pair<Vec3f, Vec2f>> plate_triangles(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed contour.
     *
     * @param bed The bed whose contour is required.
     *
     * @return the geometry of the bed contour as a std::vector of vertices, two vertices for each segment.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<Vec3f> plate_contour(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed print volume.
     *
     * @param bed The bed whose print volume is required.
     *
     * @return the geometry of the bed print volume as a std::vector of vertices, two vertices for each segment.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<Vec3f> print_volume(const Domain::Bed& bed);
};

} // namespace Slic3r::Biz::Plater