#pragma once

#include <functional>
#include <string>

namespace Slic3r {

namespace Domain
{
    class Project;
}

namespace Biz
{
    void load_project(const std::string& file_path, std::function<void(Slic3r::Domain::Project&&)> after_load);
}

} // namespace Slic3r
