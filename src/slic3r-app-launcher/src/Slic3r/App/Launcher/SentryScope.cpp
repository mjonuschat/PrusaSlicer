#include "SentryScope.hpp"
#include <Slic3r/Log.hpp>
#include <boost/filesystem.hpp>

#ifdef SLIC3R_SENTRY
#include "Slic3r/Version.hpp"
#include <sentry.h>
#include <fmt/format.h>
#include "Slic3r/Biz/Network/HttpCurl.hpp"
#include "Slic3r/Directories.hpp"
#endif

namespace Slic3r::App::Launcher {

#ifdef SLIC3R_SENTRY
namespace fs = boost::filesystem;
static fs::path get_database_path() {
    const fs::path cache_path{cache_dir()};
    return cache_path / ".sentry-native";
}
#endif

SentryScope::SentryScope()
{
#ifdef SLIC3R_SENTRY
#ifdef __linux__
    Biz::Network::HttpCurl::tls_global_init();
#endif

    static std::string release = fmt::format("{}@{}", APP_NAME, VERSION);

    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, SLIC3R_SENTRY_DSN);
    // This is also the default-path. For further information and recommendations:
    // https://docs.sentry.io/platforms/native/configuration/options/#database-path

    const fs::path database_path{get_database_path()};
    sentry_options_set_database_path(options, database_path.c_str());
    sentry_options_set_release(options, release.c_str());
    sentry_options_set_debug(
        options,
#ifdef _NDEBUG
        0
#else
        1
#endif
    );
    const auto& file_logging = get_file_log_config();
    if (!file_logging.log_file.empty()) {
        sentry_options_add_attachment(options, file_logging.log_file.c_str());
    }
    sentry_init(options);
#endif
}

SentryScope::~SentryScope()
{
#ifdef SLIC3R_SENTRY
    // make sure everything flushes
    sentry_close();
#endif
}

void SentryScope::send_test_message()
{
#ifdef SLIC3R_SENTRY
    sentry_capture_event(sentry_value_new_message_event(
        /*   level */ SENTRY_LEVEL_INFO,
        /*  logger */ "custom",
        /* message */ "It works!"
    ));
#endif
}

} // namespace Slic3r::App::Launcher
