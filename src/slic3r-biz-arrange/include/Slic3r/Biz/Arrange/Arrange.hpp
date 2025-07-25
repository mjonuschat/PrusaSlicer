#pragma once

#include "Slic3r/Biz/Arrange/ArrangeItem.hpp"

namespace Slic3r::Biz::Arrange {

struct ArrangeResult
{
    std::vector<ArrangeItem> packed;
    std::vector<ArrangeItem> not_packed;
};

std::vector<ArrangeItem> to_arrange_items(const std::vector<InputShape>& items, const Settings& settings);

ArrangeResult arrange(
    const Domain::Points& bed_contour,
    std::vector<ArrangeItem>& items,
    const std::vector<ArrangeItem>& fixed_items,
    const Settings& settings
);

} // namespace Slic3r::Biz::Arrange
