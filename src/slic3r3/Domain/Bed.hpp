//
// Created by Jan Bartipan on 22.02.2024.
//

#pragma once

#include <memory>

namespace Slic3r {
class Model;
}

namespace Slic3r::Domain {

class Bed
{
public:
private:
    std::unique_ptr<Model> m_model;

    // other bed data like grid position
};

} // namespace Slic3r::Domain
