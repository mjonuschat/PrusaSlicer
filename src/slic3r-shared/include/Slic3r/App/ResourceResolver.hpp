///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/App/Render/IResourceResolver.hpp>

#include <boost/filesystem/path.hpp>

namespace Slic3r::App {

class ResourceResolver : public Render::IResourceResolver
{
public:
    explicit ResourceResolver(const std::string& resource_path);

    std::string resolve(const std::string& relative_filepath) override;

private:
    boost::filesystem::path m_resources_path;
};

} // namespace Slic3r::App
