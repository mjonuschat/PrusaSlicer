#include "Slic3r/App/Lua/PluginRegistry.hpp"

#include "spdlog/spdlog.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/directory.hpp>

namespace Slic3r::App::Lua {

void PluginRegistry::scan(const std::string& path)
{
    namespace fs = boost::filesystem;

    fs::path p(path);
    if (!fs::exists(p)) {
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator{p}) {
        if (entry.path().extension() != ".lua") {
            continue;
        }

        Biz::Lua::LuaEngine lua;
        auto entry_path = entry.path().string();
        sol::protected_function_result ret;
        try {
            ret = lua.run_file(entry_path);
        } catch (std::exception& e) {
            SPDLOG_ERROR("Failed loading plugin: {}", e.what());
        }
        if (!ret.valid()) {
            sol::error err = ret;
            SPDLOG_ERROR("Failed loading plugin {}: {}", entry_path, err.what());
        }
        auto result = Plugin::parse(lua, entry_path);
        if (result) {
            auto&& plugin = result.value();
            std::string plugin_id{plugin.meta().id};
            std::string plugin_path{plugin.path()};

            auto [it, inserted] = m_plugins.emplace(plugin.meta().id, std::move(plugin));
            if (!inserted) {
                SPDLOG_ERROR(
                    "Plugin ID: {} of {} already registered by {}",
                    plugin_id,
                    plugin_path,
                    it->second.path()
                );
            }
        } else {
            SPDLOG_INFO(
                "Parsing metadata of plugin {} unsuccessful: {}\n"
                "This may not be an error if the .lua file is not a plugin but shared module.",
                entry_path,
                result.error()
            );
        }
    }
}


} // namespace Slic3r::App::Lua
