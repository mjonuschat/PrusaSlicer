#pragma once

#include <map>
#include <string>

#include "Slic3r/App/Lua/Plugin.hpp"

namespace Slic3r::App::Lua {

class PluginRegistry
{
public:
    using Plugins = std::map<std::string, Plugin>;

    void clear() { m_plugins.clear(); }
    void scan(const std::string& path);
    const Plugins& plugins() const { return m_plugins; }

private:
    Plugins m_plugins;
};

}