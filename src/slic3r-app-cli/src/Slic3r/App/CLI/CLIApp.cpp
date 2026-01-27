#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/CLI/LoadPrintData.hpp"
#include "Slic3r/App/CLI/ProcessActions.hpp"
#include "Slic3r/App/CLI/ProcessTransform.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/Biz/IMessageDialogProvider.hpp"
#include "Slic3r/App/Init.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/Project.hpp"

namespace Slic3r::Biz::Preset {
struct PresetSelectionNames;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::CLI {

// Dummy implementation to pass 3MF loading and other steps.
class CLIDummyDialogManager : public IDialogManager
{
public:
    void show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title,
        const boost::filesystem::path& default_folder,
        const std::string& default_file_name,
        const std::string& wildcards,
        const FileCallback& callback
    ) override
    {}

    std::string show_input_dialog(
        const std::string& title,
        const std::string& text,
        const std::string& default_value
    ) override 
    { return {}; }

    void show_input_dialog_with_buttons(
        const std::string& title,
        const std::string& text,
        const std::string& default_value,
        const std::vector<ButtonWithCallback>& buttons
    ) override
    {}

    std::string show_combo_dialog(
        const std::string& title,
        const std::string& text,
        const std::vector<std::string>& values
    ) override  { return {}; }

    void show_webview_dialog(
        std::unique_ptr<Browser::AbstractBrowserLogic>&& logic,
        Biz::ProjectInteractor* project_interactor
    ) override
    {}

    void show_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const YesNoCallback& callback
    ) override
    {}

    void show_rich_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const std::string& check_text,
        const YesNoCallback& callback,
        const CheckBoxCheckedCallback& checked_callback
    ) override
    {}

    void show_diff_dialog(
        const Biz::Preset::PresetInteractor& preset_interactor,
        std::optional<Domain::Preset::PresetKind> kind
    ) override
    {}

    std::string show_ramming_dialog(const std::string& ramming_parameters) override
    {
        PANIC("Ramming dialog not implemented for CLI");
    }

    void open_in_browser(const std::string& link, int flag) override
    {
        PANIC("Open browser not implemented for CLI");
    }

    void show_info_dialog(const std::string& text, const std::string& title, bool is_marked = false) override
    {
        SPDLOG_INFO("{}: {}", title, text);
    }

    void show_warning_dialog(const std::string& text, const std::string& title) override
    {
        SPDLOG_WARN("{}: {}", title, text);
    }

    void show_error_dialog(const std::string& text, const std::string& title) override
    {
        SPDLOG_ERROR("{}: {}", title, text);
    }

    PresetsSwitchStates show_unsaved_changes_dialog(
        const std::string& dialog_name,
        const Domain::ConfigPack& config_original,
        const Domain::ConfigPack& config_selected,
        Domain::ConfigPack* config_new_selected,
        const Biz::Preset::PresetSelectionNames& preset_names,
        const Biz::Preset::PresetSelectionNames& preset_names_new,
        const Biz::Preset::PresetInteractor& preset_interactor,
        bool new_printer_has_multiple_extruders
    ) override
    {
        return {};
    }

    std::optional<Biz::StepLoadDialogResult> show_load_step_dialog(
        const std::string& filename,
        double linear_precision,
        double angle_precision,
        bool multiple) override
    {
        return std::nullopt;
    }

    std::string show_save_dialog(
        Domain::Preset::PresetKind kind,
        const std::string& original_name,
        const Biz::Preset::PresetInteractor& preset_interactor
    ) override
    {
        return "";
    }

};

int run(InitParams& init_params)
{
    // Even CLI needs to initialize DialogManager to pass 3MF loading and other steps.
    AppServices& app_services = AppServices::instance();
    app_services.set_dialog_manager(std::make_unique<CLIDummyDialogManager>());

    if (process_profiles_sharing(init_params)) {
        return EXIT_SUCCESS;
    }

    std::optional<Domain::PrinterTechnology> printer_technology =
        get_printer_technology(init_params);

    Domain::ConfigPack config_pack;
    std::vector<Domain::Project> projects;

    if (!load_print_data(projects, config_pack, printer_technology, init_params)) {
        return EXIT_FAILURE;
    }

    if (is_needed_post_processing(config_pack)) {
        return EXIT_SUCCESS;
    }

    if (!process_transform(init_params, config_pack, projects)) {
        return EXIT_FAILURE;
    }

    if (!process_actions(init_params, config_pack, projects)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
} // namespace Slic3r::App::CLI
