#include "SentryScope.hpp"
#include <Slic3r/Log.hpp>

#ifdef SLIC3R_SENTRY
#include "Slic3r/Version.hpp"
#include <sentry.h>
#include <fmt/format.h>
#endif


namespace Slic3r::App::Launcher {

#ifdef SLIC3R_SENTRY
constexpr bool SENTRY_ENABLED = true;
#else
constexpr bool SENTRY_ENABLED = false;
#endif // SLIC3R_SENTRY

SentryScope::SentryScope()
{
#ifdef SLIC3R_SENTRY
    static std::string release = fmt::format("{}@{}", APP_NAME, VERSION);

    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, SLIC3R_SENTRY_DSN);
    // This is also the default-path. For further information and recommendations:
    // https://docs.sentry.io/platforms/native/configuration/options/#database-path
    sentry_options_set_database_path(options, ".sentry-native");
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
