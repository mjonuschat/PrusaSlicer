#ifndef libslic3r_MultipleBeds_hpp_
#define libslic3r_MultipleBeds_hpp_

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/ObjectID.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"


namespace Slic3r {

constexpr size_t MAX_NUMBER_OF_BEDS = 9;


namespace BedsGrid {
using GridCoords = std::array<int, 2>;
using Index = int;
Index grid_coords2index(const GridCoords &coords);
GridCoords index2grid_coords(Index index);
}


class MultipleBeds {
public:
	MultipleBeds() = default;

	static constexpr int get_max_beds() { return MAX_NUMBER_OF_BEDS; };
	Domain::Vec3d get_bed_translation(int id) const;

	int    get_number_of_beds() const   { return m_number_of_beds; }
	int    get_active_bed() const       { return m_active_bed; }
	
	void   set_loading_project_flag(bool project) { m_loading_project = project; }
	bool   get_loading_project_flag() const { return m_loading_project; }

    Domain::Vec2d get_bed_size() const { return Biz::Algorithms::BoundingBox::sizes(m_build_volume_bb); }
    Domain::BoundingBox2crd get_bed_box() const
    {
        using Slic3r::Biz::Algorithms::Point::round;
        return Domain::BoundingBox2crd(
            round(Domain::Vec2d{m_build_volume_bb.min.x(), m_build_volume_bb.min.y()}).cast<Domain::coord_t>(),
            round(Domain::Vec2d{m_build_volume_bb.max.x(), m_build_volume_bb.max.y()}).cast<Domain::coord_t>()
        );
    }
    Domain::Vec2d bed_gap() const;
    Domain::Vec2crd get_bed_gap() const;

private:
	int m_number_of_beds = 1;
	int m_active_bed     = 0;
	Domain::BoundingBox2d m_build_volume_bb;
	bool m_legacy_layout = false;
	bool m_loading_project = false;
};

extern MultipleBeds s_multiple_beds;

} // namespace Slic3r

#endif // libslic3r_MultipleBeds_hpp_
