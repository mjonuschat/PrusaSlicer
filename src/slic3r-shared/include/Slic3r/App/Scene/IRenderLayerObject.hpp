#pragma once

#include <cstdint>

namespace Slic3r::App::Scene {

class IRenderLayerObject
{
public:
    virtual ~IRenderLayerObject() = default;

    virtual int layer_index() const = 0;
};

}
