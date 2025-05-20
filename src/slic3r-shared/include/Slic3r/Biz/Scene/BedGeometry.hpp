#pragma once

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"

namespace Slic3r::Domain {
class Bed;
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Scene {

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
    [[nodiscard]] static Domain::TriangleMesh model(const Domain::Bed& bed);

    /**
     * @brief Return the geometry (triangulated contour) of the bed plate.
     *
     * @param bed The bed whose plate is required.
     *
     * @return the geometry of the bed plate as a std::vector of pairs vertex-uvs, three pairs for each triangle.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<std::pair<Domain::Vec3f, Domain::Vec2f>> plate_triangles(const Domain::Bed& bed);

    /**
     * @brief Return the geometry (triangulated contour) of the bed plate as mesh.
     *
     * @param bed The bed whose plate is required.
     *
     * @return the geometry of the bed plate as TriangleMesh.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static Domain::TriangleMesh plate_mesh(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed contour.
     *
     * @param bed The bed whose contour is required.
     *
     * @return the geometry of the bed contour as a std::vector of vertices, two vertices for each segment.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<Domain::Vec3f> plate_contour(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed print volume.
     *
     * @param bed The bed whose print volume is required.
     *
     * @return the geometry of the bed print volume as a std::vector of vertices, two vertices for each segment.
     *
     * @note The bed contour is specified into the bed, see Slic3r::Domain::Bed definition.
     */
    [[nodiscard]] static std::vector<Domain::Vec3f> print_volume(const Domain::Bed& bed);

    /**
     * @brief Return the geometry of the bed axis arrow.
     * 
     * @param bed The bed whose model is required.
     *
     * @return the geometry of the bed axis arrow as TriangleMesh.
     * 
     * @note The generated arrow is pointing in the positive Z axis direction.
     */
    [[nodiscard]] static Domain::TriangleMesh axis(const Domain::Bed& bed);
};

} // namespace Slic3r::Biz::Scene
