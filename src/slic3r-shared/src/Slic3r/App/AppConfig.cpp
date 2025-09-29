#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Biz/Config/ConfigLoad.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Assert.hpp"

#include "nlohmann/json.hpp"
#include "boost/filesystem.hpp"
#include "boost/system/error_code.hpp"
#include "boost/nowide/fstream.hpp"

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

    def = defs.add("show_splash_screen", typeid(bool));
    def->location = Domain::AppConfigLocation{};
    def->label = L("Show splashscreen");
    def->tooltip = L("Show splashscreen during application start.");
    def->init_fn = []() { return Domain::ConfigValue(true); };
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



} // namespace Slic3r::App
