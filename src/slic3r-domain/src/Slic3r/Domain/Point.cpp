#include "Slic3r/Domain/Point.hpp"


namespace Slic3r::Domain {

namespace Impl {

// TODO: Legacy, maybe we should look into it.
coord_t fast_round_up(double a)
{
    // Why does Java Math.round(0.49999999999999994) return 1?
    // https://stackoverflow.com/questions/9902968/why-does-math-round0-49999999999999994-return-1
    return a == 0.49999999999999994 ? coord_t{0} : static_cast<coord_t>(std::floor(a + 0.5));
}
}

void Point::rotate(double angle, const Point &center)
{
    Vec2d  cur = this->cast<double>();
    double s   = ::sin(angle);
    double c   = ::cos(angle);
    auto   d   = cur - center.cast<double>();
    this->x() = Impl::fast_round_up(center.x() + c * d.x() - s * d.y());
    this->y() = Impl::fast_round_up(center.y() + s * d.x() + c * d.y());
}

Vec2crd rotated(const Vec2crd& point, const double angle, const Vec2crd &center) {
    const Vec2d current{point.cast<double>()};
    const double s{std::sin(angle)};
    const double c{std::cos(angle)};
    const auto d{current - center.cast<double>()};
    return {
        Impl::fast_round_up(center.x() + c * d.x() - s * d.y()),
        Impl::fast_round_up(center.y() + s * d.x() + c * d.y())
    };
}
}
