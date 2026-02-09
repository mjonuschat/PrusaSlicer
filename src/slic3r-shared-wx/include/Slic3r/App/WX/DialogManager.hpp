#pragma once 

#include <Slic3r/App/IDialogManager.hpp>

#include <boost/filesystem/path.hpp>

namespace Slic3r::App::WX {

class DialogManager : public IDialogManager{
public:
    DialogManager() = default;

    void show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title, 
        const boost::filesystem::path& override_dir,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const FileCallback& callback
    ) override;

    void show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Slic3r::Biz::ProjectInteractor* project_interactor) override;
    void show_yesno_dialog(const std::string& title, const std::string& text, const YesNoCallback& callback) override;
    void show_rich_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const std::string& check_text,
        const YesNoCallback& callback,
        const CheckBoxCheckedCallback& checked_callback
    ) override;
    void show_info_dialog(const std::string& text, const std::string& title = std::string(), bool is_marked = false) override;
    void show_warning_dialog(const std::string& text, const std::string& title = std::string()) override;
    void show_error_dialog(const std::string& text, const std::string& title = std::string()) override;
    void show_diff_dialog(const Slic3r::Biz::Preset::PresetInteractor& preset_interactor, std::optional<Domain::Preset::PresetKind> kind = std::nullopt) override;
    PresetsSwitchStates show_unsaved_changes_dialog(
        const std::string& dialog_name,
        const Domain::ConfigPack& config_original,
        const Domain::ConfigPack& config_selected,
        Domain::ConfigPack* config_new_selected,
        const Slic3r::Biz::Preset::PresetSelectionNames& preset_names,
        const Slic3r::Biz::Preset::PresetSelectionNames& preset_names_new
    ) override;
    std::string show_ramming_dialog(const std::string& ramming_parameters) override;
    std::optional<Biz::StepLoadDialogResult> show_load_step_dialog(
        const std::string& filename,
        double linear_precision,
        double angle_precision,
        bool multiple
    ) override;

private:
    boost::filesystem::path m_last_dir;
};

} //namespace Slic3r::App::WX