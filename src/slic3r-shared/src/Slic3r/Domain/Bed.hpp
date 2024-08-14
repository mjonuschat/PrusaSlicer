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
    [[nodiscard]] ModelInstances & model_instances() { return m_instances; }
    [[nodiscard]] const ModelInstances& model() const { return m_instances; }
private:
    ModelInstances m_instances;

    // other bed data like grid position
};

} // namespace Slic3r::Domain
