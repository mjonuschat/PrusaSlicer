#pragma once
#include "Slic3r/App/Scene/Transform.hpp"

namespace Slic3r::App::Scene {

class INodeTransformModifier {
public:
    virtual ~INodeTransformModifier() = default;

    virtual void modify_world_transform(Transform& xform) = 0;

};

}
