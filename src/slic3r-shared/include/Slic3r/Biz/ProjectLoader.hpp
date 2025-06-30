#pragma once

#include <functional>
#include <string>
#include <exception>

namespace Slic3r {

namespace Domain
{
    class Project;
}

namespace Biz
{
    void load_project(const std::string& file_path, std::function<void(Domain::Project&&)> after_load);
}

} // namespace Slic3r
