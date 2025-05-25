///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ResourceResolver.hpp"

#include <Slic3r/Log.hpp>

#include <boost/filesystem/operations.hpp>

namespace Slic3r::App {

ResourceResolver::ResourceResolver(const std::string& resource_path)
    : m_resources_path(resource_path)
{}

std::string ResourceResolver::resolve(const std::string& relative_filepath)
{
    boost::filesystem::path path = boost::filesystem::exists(relative_filepath)
        ? relative_filepath
        : (m_resources_path / "icons" / relative_filepath);

    return path.string();
}

} // namespace Slic3r::App
