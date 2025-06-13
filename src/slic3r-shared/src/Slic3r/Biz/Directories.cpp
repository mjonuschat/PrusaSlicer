#include "Slic3r/Biz/Directories.hpp"

#include "Slic3r/Log.hpp"

#include "libslic3r/libslic3r.h"

#include <boost/filesystem/path.hpp>
#include <boost/nowide/convert.hpp>

#if defined(_WIN32)

#include <shlobj.h>

#elif defined(__linux__)

#include <stdlib.h>
#include <pwd.h>

#endif

namespace Slic3r::Biz::Utils {

static std::string g_var_dir;

void set_var_dir(const std::string &dir)
{
    g_var_dir = dir;
}

const std::string& var_dir()
{
    return g_var_dir;
}

std::string var(const std::string &file_name)
{
    auto file = (boost::filesystem::path(g_var_dir) / file_name).make_preferred();
    return file.string();
}

static std::string g_resources_dir;

void set_resources_dir(const std::string &dir)
{
    g_resources_dir = dir;
}

const std::string& resources_dir()
{
    return g_resources_dir;
}

static std::string g_local_dir;

void set_local_dir(const std::string &dir)
{
    g_local_dir = dir;
}

const std::string& localization_dir()
{
	return g_local_dir;
}

static std::string g_sys_shapes_dir;

void set_sys_shapes_dir(const std::string &dir)
{
    g_sys_shapes_dir = dir;
}

const std::string& sys_shapes_dir()
{
	return g_sys_shapes_dir;
}

static std::string g_custom_gcodes_dir;

void set_custom_gcodes_dir(const std::string &dir)
{
    g_custom_gcodes_dir = dir;
}

const std::string& custom_gcodes_dir()
{
    return g_custom_gcodes_dir;
}

static std::string g_data_dir;

void set_data_dir(const std::string &dir)
{
    g_data_dir = dir;
}

const std::string& data_dir()
{
    return g_data_dir;
}

std::string custom_shapes_dir()
{
    return (boost::filesystem::path(g_data_dir) / "shapes").string();
}

#if defined(_WIN32)

static std::string GetDataDir()
{
    HRESULT hr = E_FAIL;

    std::wstring buffer;
    buffer.resize(MAX_PATH);

    hr = ::SHGetFolderPathW
    (
        NULL,               // parent window, not used
        CSIDL_APPDATA,
        NULL,               // access token (current user)
        SHGFP_TYPE_CURRENT, // current path, not just default value
        (LPWSTR)buffer.data()
    );

    if (hr == E_FAIL)
    {
        // directory doesn't exist, maybe we can get its default value?
        hr = ::SHGetFolderPathW
        (
            NULL,
            CSIDL_APPDATA,
            NULL,
            SHGFP_TYPE_DEFAULT,
            (LPWSTR)buffer.data()
        );
    }

    for (int i=0; i< MAX_PATH; i++)
        if (buffer.data()[i] == '\0') {
            buffer.resize(i);
            break;
        }

    return  boost::nowide::narrow(buffer);
}

#elif defined(__linux__)

std::optional<std::string> get_env(std::string_view key) {
    const char* result{getenv(key.data())};
    if(result == nullptr) {
        return std::nullopt;
    }
    return std::string{result};
}

namespace {
std::optional<boost::filesystem::path> get_home_dir(const std::string& subfolder) {
    if (auto result{get_env("HOME")}) {
        return *result + subfolder;
    } else {
        std::optional<std::string> user_name{get_env("USER")};
        if (!user_name) {
            user_name = get_env("LOGNAME");
        }
        struct passwd* who{
            user_name ?
            getpwnam(user_name->data()) :
            (struct passwd*)NULL
        };
        // make sure the user exists!
        if (!who) {
            who = getpwuid(getuid());
        }
        if (who) {
            return std::string{who->pw_dir} + subfolder;
        }
    }
    return std::nullopt;
}
}

namespace Slic3r {
std::optional<boost::filesystem::path> get_home_config_dir() {
    return get_home_dir("/.config");
}

std::optional<boost::filesystem::path> get_home_local_dir() {
    return get_home_dir("/.local");
}
}

std::string GetDataDir()
{
    if (auto result{get_env("XDG_CONFIG_HOME")}) {
        return *result;
    } else if (auto result{Slic3r::get_home_config_dir()}) {
        return result->string();
    }

    SPDLOG_ERROR("GetDataDir() > unsupported file layout");

    return {};
}

#elif defined(__EMSCRIPTEN__)
static std::string GetDataDir()
{
    // Emscripten TODO: provide better data dir path
    std::string dir;
    return dir;
}
#endif

std::string get_default_datadir()
{
    const std::string config_dir = GetDataDir();
    std::string datadir_name = std::string(SLIC3R_APP_FULL_NAME) + "3";
    std::string data_dir = (boost::filesystem::path(config_dir) / datadir_name).make_preferred().string();
    return data_dir;
}

}