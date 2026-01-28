#pragma once

#include <string>
#include "tl/expected.hpp"
#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::App {

enum class GraphicsQuality
{
    Legacy,
    Low,
    Medium,
    High
};

class AppSettings : public Domain::ConfigBox
{
public:
    AppSettings();
};

class AppConfig {
public:
    AppConfig() = default;
    ~AppConfig() { save(); }

    static std::unique_ptr<AppConfig> create_app_config();

    void set_filename(const std::string& filename) { m_filename = filename; }
    
    bool save() const;

    template<typename T>
    T get(const std::string_view key) const {
        return m_app_settings.items.opt(key).get<T>();
    }

    template<typename T>
    void set(const std::string_view key, T value) {
        m_app_settings.items.opt(key).set(value);
    }

    const AppSettings& get_config_box() const { return m_app_settings; }
    AppSettings* get_config_box_ptr() { return &m_app_settings; }

    bool is_printables_enabled() const;
    bool is_prusa_account_enabled() const;

    /**
     * @brief Records a crash and disables a related configuration option.
     *
     * @param crash_reason Identifier of the detected crash cause.
     * @param disabled_option_key Configuration option to disable to prevent repeated crashes.
     */
    void record_crash(const std::string& crash_reason, const std::string& disabled_option_key);

    /**
     * @brief Resolves a previously recorded crash.
     *
     * Clears or updates the stored crash reason and optionally restores
     * a previously disabled configuration option.
     *
     * @param resolved_crash_reason New crash reason to store.
     *                              If empty, the crash state is cleared.
     * @param restored_option_key   Configuration option to re-enable.
     *                              If empty, no option is restored.
     */
    void resolve_crash(const std::string& resolved_crash_reason = std::string(),
        const std::string& restored_option_key = std::string());


private:
    AppSettings m_app_settings;
    std::string m_filename;

    /**
     * @brief Loads the application configuration from the specified file.
     *
     * If the file is not found, a default AppConfig is returned.
     * If the file is present but cannot be read, an error is returned.
     *
     * @param path The path to the configuration file.
     * @return A tl::expected containing the AppConfig on success, or an error message on failure.
     */
    static tl::expected<AppConfig, std::string> load_appconfig(const std::string& filename);
};

} // namespace Slic3r::App
