#pragma once

#include "Slic3r/App/Lua/Plugin.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include <tl/expected.hpp>

#include "Slic3r/Semver.hpp"
#include "Slic3r/Biz/Crypto/ContentProvider.hpp"
#include "Slic3r/Biz/Crypto/Sign.hpp"

namespace Slic3r::App::Lua {
enum class PluginApiType
{
    Project
};


std::string to_string(PluginApiType pat);
tl::expected<PluginApiType, std::string> parse_plugin_api_type(std::string_view s);

struct PluginBundleMeta
{
    using OptString = std::optional<std::string>;
    using ApiVersionMap = std::map<PluginApiType, Semver>;

    std::string id;
    Semver version;
    std::string author;
    OptString description;
    ApiVersionMap required_apis;
};


using PluginInstallResult = tl::expected<void, std::string>;

class PluginBundle
{
public:
    using Plugins = std::vector<Plugin>;

    explicit PluginBundle(Biz::Crypto::IContentProviderPtr&& content_provider) : m_content_provider(std::move(content_provider)) {}

    PluginInstallResult load_meta();

    const PluginBundleMeta& meta() const { return m_meta; }

    bool verify(const Biz::Crypto::KeyPair& author_pub_key) const;
    PluginInstallResult install(const std::string& dest_base_dir);

    Plugins load_plugins();

    static bool is_plugin_bundle_dir(const std::string& path);

private:
    PluginBundleMeta m_meta;
    Biz::Crypto::IContentProviderPtr m_content_provider;
};

}
