#include "Slic3r/App/PresetUpdaterCLI.hpp"

#include "Slic3r/Log.hpp"
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r::Biz::PresetUpdater {
void to_json(nlohmann::json& j, const PresetUpdaterWarning& w) {
    j = nlohmann::json{
        {"text", w.text},
        {"repo", w.repo},
        {"vendor", w.vendor}
    };
}

void from_json(const nlohmann::json& j, PresetUpdaterWarning& w) {
    j.at("text").get_to(w.text);
    j.at("repo").get_to(w.repo);
    j.at("vendor").get_to(w.vendor);
}
}

namespace Slic3r::App {

PresetUpdaterCLI::PresetUpdaterCLI(Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor) :
    m_preset_updater_interactor(preset_updater_interactor)
{
    m_preset_updater_interactor.add_listener<Biz::PresetUpdater::IPresetUpdaterResultListener>(
        this
    );
}

void PresetUpdaterCLI::start(const ActionParams& action, const std::string data)
{
    if (action.preset_updater_sync) {
        m_preset_updater_interactor.build_update_sync_and_reconfiguration_check();
    } else if (action.preset_updater_perform) {
        m_perform_after_sync = true;
        m_preset_updater_interactor.build_update_sync_and_reconfiguration_check();
    } else if (action.preset_updater_add_local) {
        ASSERT(!data.empty());
        boost::filesystem::path path(data);
        m_preset_updater_interactor.add_local_repository(path, true);
    } else if (action.preset_updater_remove_local) {
        ASSERT(!data.empty());
        m_preset_updater_interactor.remove_local_repository(data);
    } else if (action.preset_updater_list_repos) {
        m_preset_updater_interactor.update_repositories(
            Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector{}
        );
    } else if (action.preset_updater_switch_repo) {
        ASSERT(!data.empty());
        m_repo_to_switch = data;
        m_preset_updater_interactor.list_repositories();
    } else if (action.preset_updater_cleanup) {
        m_preset_updater_interactor.cleanup_update_sync();
    } else {
        ASSERT(false);
    }
}

void PresetUpdaterCLI::on_preset_updater_error(const std::string& body)
{
    nlohmann::json j = {{"error", body}};
    printf(j.dump(-1).c_str());
    printf("\n");
    m_has_result = true;
}

void PresetUpdaterCLI::on_preset_updater_status(const std::string& target, int attempt, unsigned delay)
{
    SPDLOG_INFO("target:{} attempt:{} delay:{}", target, attempt, delay);
}

void PresetUpdaterCLI::on_preset_updater_reconfigurations_list(
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (m_perform_after_sync) {
        m_perform_after_sync = false;
        m_preset_updater_interactor.perform_reconfigurations(reconfigurations);
    } else {
        if (!warnings.empty()) {
            nlohmann::json j = {{"result", reconfigurations},{"warnings", warnings}};
            printf(j.dump(4).c_str());
            printf("\n");
        } else {
            nlohmann::json j = {{"result", reconfigurations}};
            printf(j.dump(4).c_str());
            printf("\n");
        }
        m_has_result = true;
    }
}

void PresetUpdaterCLI::on_preset_updater_reconfigurations_perfomed(
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    if (!warnings.empty()) {
        nlohmann::json j = {{"warnings", warnings}};
        printf(j.dump(4).c_str());
        printf("\n");
    }
    m_has_result = true;
}

void PresetUpdaterCLI::on_preset_updater_repository_info_vector(
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    SPDLOG_INFO(__FUNCTION__);
    if (!m_repo_to_switch.empty()) {
        Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector updated_descriptor(
            descriptor
        );
        // find repo to switch
        bool is_selected_after_switch = false;
        std::string repo_id;
        for (auto& repo : updated_descriptor) {
            if (repo.descriptor.uuid == m_repo_to_switch) {
                repo.selected            = !repo.selected;
                is_selected_after_switch = repo.selected;
                repo_id                  = repo.descriptor.id;
                break;
            }
        }
        if (is_selected_after_switch) {
            // deselect all repos with same id
            for (auto& repo : updated_descriptor) {
                if (repo.descriptor.id == repo_id && repo.descriptor.uuid != m_repo_to_switch) {
                    repo.selected = false;
                }
            }
        }
        m_repo_to_switch.clear();
        m_preset_updater_interactor.update_repositories(updated_descriptor);
    } else {
        if (!warnings.empty()) {
            nlohmann::json j = {{"result", descriptor},{"warnings", warnings}};
            printf(j.dump(4).c_str());
            printf("\n");
        } else {
            nlohmann::json j = {{"result", descriptor}};
            printf(j.dump(4).c_str());
            printf("\n");
        }
        m_has_result = true;
    }
}

} // namespace Slic3r::App
