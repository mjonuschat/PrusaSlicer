#pragma once

#include "Slic3r/Biz/Arrange/Bed.hpp"
#include "Slic3r/Biz/Arrange/ArrangeItem.hpp"

namespace Slic3r::Biz::Arrange {

struct ArrangeResult
{
    std::vector<ArrangeItem> packed;
    std::vector<ArrangeItem> not_packed;
};

ArrangeResult arrange(
    const IBed& bed,
    const std::vector<ArbitraryShape>& items,
    const std::vector<ArbitraryShape>& fixed_items,
    const Settings& settings
);

} // namespace Slic3r::Biz::Arrange
