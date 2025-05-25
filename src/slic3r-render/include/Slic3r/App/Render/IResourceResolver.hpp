///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string>

namespace Slic3r::App::Render {
class IResourceResolver {
public:

    /**
     * Resolves relative_filepath into absolute one.
     * @param relative_filepath of the file to be resolved
     * @return either absolute filepath or empty resolved string
     */
    virtual std::string resolve(const std::string& relative_filepath) = 0;
};

}
