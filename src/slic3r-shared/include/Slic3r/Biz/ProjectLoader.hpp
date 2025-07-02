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
    void load_project(
        const std::string& file_path,
        std::function<void(Domain::Project&&)> after_load,
        std::function<void(std::exception_ptr)> after_exception
    );
}

} // namespace Slic3r
