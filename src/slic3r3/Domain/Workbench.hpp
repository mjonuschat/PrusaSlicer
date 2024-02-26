#pragma once

#include "Project.hpp"

namespace Slic3r::Domain {

class Workbench
{
public:
    [[nodiscard]] Project& project() { return m_project; }
    [[nodiscard]] const Project& project() const { return m_project; }
private:
    Project m_project;
};

} // namespace Slic3r::Domain
