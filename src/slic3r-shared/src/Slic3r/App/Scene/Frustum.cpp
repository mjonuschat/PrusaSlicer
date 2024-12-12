#include "Slic3r/App/Scene/Frustum.hpp"

namespace Slic3r::App::Scene {

bool Frustum::intersects(const Eigen::AlignedBox3d& box) const
{
		if (!intersects(box.center(), 0.5 * box.diagonal().norm()))
				return false;

		size_t inside = 0;
		for (size_t i = 0; i < planes.size(); ++i) {
				inside = 0;
				for (size_t j = 0; j < 8 && inside == 0; ++j) {
						if (planes[i].signed_distance(box.corner(Eigen::AlignedBox3d::CornerType(j))) >= 0.0)
								++inside;
				}
				if (inside == 0)
						return false;
		}
		return true;
}

bool Frustum::intersects(const Vec3d& center, double radius) const
{
    for (size_t i = 0; i < planes.size(); ++i) {
	      if (planes[i].signed_distance(center) < -radius)
  		      return false;
    }
    return true;
}

} // namespace Slic3r::App::Scene
