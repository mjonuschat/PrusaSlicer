#include "Slic3r/App/AppConfig.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Config/ConfigLoad.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Directories.hpp"

#include "nlohmann/json.hpp"
#include "boost/filesystem.hpp"
#include "boost/system/error_code.hpp"
#include "boost/nowide/fstream.hpp"

using namespace Slic3r::Biz;

namespace Slic3r::App {

    void appconfig_config_init_fn(Domain::ConfigDefinitions& defs);

static const Domain::ConfigDefinitions& get_defs_appconfig()
{
    static Domain::ConfigDefinitions defs_appconfig(
        {Domain::AppConfigLocation{}}, appconfig_config_init_fn
    );
    return defs_appconfig;
}

AppSettings::AppSettings() : ConfigBox(get_defs_appconfig(), Domain::AppConfigLocation{}) {}

void appconfig_config_init_fn(Domain::ConfigDefinitions& defs)
{
    Domain::ConfigItemDef* def = nullptr;

    using Category = Domain::ConfigItemDef::Category;
    using GUIType = Domain::ConfigItemDef::GUIType;

    def = defs.add("show_splash_screen", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Show splashscreen");
    def->tooltip = L("Show splashscreen during application start.");
    def->category = Category::PreferencesGeneral;
    def->option_group = L("Application");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("restore_win_position", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Restore window position on start");
    def->tooltip = L("Restore window position on start.");
    def->category = Category::Hidden;// PreferencesGeneral;
    def->option_group = L("Application");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("allow_web_services", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Allow Web Services");
    def->tooltip = L("Master switch for enabling services like Prusa Account, Prusa Connect, Printables.");
    def->category = Category::Hidden;//Services;
    def->option_group = L("Services setup");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("enable_prusa_account", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Enable Prusa Account");
    def->tooltip = L("Enable logging to Prusa Account and sending files to Connect. Works only if allow_web_services is enabled.");
    def->category = Category::Services;
    def->option_group = L("Services setup");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("enable_connect", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Enable Prusa Connect");
    def->tooltip = L("Enable Connect tab and uploading slicing results. Works only if allow_web_services and enable_prusa_account is enabled.");
    def->category = Category::Services;
    def->option_group = L("Services setup");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("enable_printables", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->gui_type = GUIType::checkbox;
    def->label = L("Enable Printables");
    def->tooltip = L("Enable Printables tab. Works only if allow_web_services is enabled.");
    def->category = Category::Services;
    def->option_group = L("Services setup");
    def->init_fn = []() { return Domain::ConfigValue(true); };

    def = defs.add("layout_main_left_column_width", typeid(double));
    def->location = Domain::AppConfigLocation{};
    def->category = Domain::ConfigItemDef::Category::Hidden;
    def->init_fn = []() { return Domain::ConfigValue(300.); };

    def = defs.add("layout_main_right_column_width", typeid(double));
    def->location = Domain::AppConfigLocation{};
    def->category = Domain::ConfigItemDef::Category::Hidden;
    def->init_fn = []() { return Domain::ConfigValue(280.); };
}
 
tl::expected<AppConfig, std::string> AppConfig::load_appconfig(const std::string& filename)
{ 
    namespace fs = boost::filesystem;
    AppConfig app_config;
    try {
        if (fs::exists(fs::path(filename))) {
            boost::nowide::ifstream file_stream(filename);
            auto json_str = std::string((std::istreambuf_iterator<char>(file_stream)),
                       std::istreambuf_iterator<char>());
            nlohmann::ordered_json json = nlohmann::json::parse(json_str);
            std::string loc = Domain::get_location_name(Domain::ConfigLocation(Domain::AppConfigLocation()));
            if (json.contains(loc)) {
                auto load_issues = Biz::Config::load_box(json[loc], app_config.m_app_settings);
            }
        }
    } catch (...) {
        return tl::unexpected(L("Unable to read application settings."));
    }
    return app_config;   
}

std::unique_ptr<AppConfig> AppConfig::create_app_config()
{
    std::unique_ptr<AppConfig> app_config;
    const boost::filesystem::path appconfig_filename = boost::filesystem::path(Slic3r::data_dir()) / "shared_runtime" / "PrusaSlicer.json";
    if (auto apc = AppConfig::load_appconfig(appconfig_filename.string()); apc)
        app_config = std::make_unique<AppConfig>(apc.value());
    else {
        SPDLOG_ERROR("Failed to parse app config. Creating a new one from default.");
        app_config = std::make_unique<AppConfig>();
    }
    app_config->set_filename(appconfig_filename.string());
    return std::move(app_config);
}

bool AppConfig::save() const
{
    if (! m_filename.empty()) {
        try {
            nlohmann::ordered_json json;
            json[Domain::get_location_name(m_app_settings.location)] = m_app_settings;
            boost::nowide::ofstream out(m_filename);
            out << json.dump(2);
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool AppConfig::is_printables_enabled() const
{
#ifndef SLIC3R_HAS_WEBKIT
    return false;
#endif // SLIC3R_HAS_WEBKIT

    return get<bool>("allow_web_services") && get<bool>("enable_printables");
}

bool AppConfig::is_connect_enabled() const
{
#ifndef SLIC3R_HAS_WEBKIT
    return false;
#endif // SLIC3R_HAS_WEBKIT

    return get<bool>("allow_web_services")
        && get<bool>("enable_prusa_account")
        && get<bool>("enable_connect");
}

bool AppConfig::is_prusa_account_enabled() const
{
#ifndef SLIC3R_HAS_WEBKIT
    return false;
#endif // SLIC3R_HAS_WEBKIT

    return get<bool>("allow_web_services") && get<bool>("enable_prusa_account");
}

} // namespace Slic3r::App
