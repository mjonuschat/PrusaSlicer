//
// Created by Jan Bartipan on 22.02.2024.
//

#pragma once

#include <memory>
#include "libslic3r/ObjectID.hpp"

namespace Slic3r {
class ModelInstance;
}

namespace Slic3r::Domain {

class Bed : public ObjectBase
{
public:
    using ModelInstances = std::vector<ModelInstance*>;
    [[nodiscard]] ModelInstances & model_instances() { return m_model_instacnes; }
    [[nodiscard]] const ModelInstances& model() const { return m_model_instacnes; }
private:
    ModelInstances m_model_instacnes;

    // other bed data like grid position
};

} // namespace Slic3r::Domain
