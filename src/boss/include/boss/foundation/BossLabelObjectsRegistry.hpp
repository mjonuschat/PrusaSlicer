#pragma once

#include <vector>

#include "libslic3r/boss/gcode/labeling/LabelObjectsPolicyContext.hpp"

namespace Slic3r::Boss {

template<class... Features>
struct BossLabelObjectsRegistry {
    static std::vector<LabelObjectsDeclaration> declare(const LabelObjectsPolicyContext &ctx)
    {
        std::vector<LabelObjectsDeclaration> result;
        (append(result, Features::declare(ctx)), ...);
        return result;
    }

private:
    static void append(std::vector<LabelObjectsDeclaration> &result, std::vector<LabelObjectsDeclaration> entries)
    {
        result.insert(result.end(), std::make_move_iterator(entries.begin()), std::make_move_iterator(entries.end()));
    }
};

} // namespace Slic3r::Boss
