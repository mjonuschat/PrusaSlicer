#include "Slic3r/Biz/ProjectLoader.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"
#include "Slic3r/Biz/Format/ResultLoad3mf.hpp"

#include "Slic3r/Domain/Project.hpp"

namespace Slic3r::Biz {


    
static Domain::Project convert_to_project(Loaded3MF&& loaded_3mf)
{
    Domain::Project project;
    project.set_file_name(loaded_3mf.filepath_3mf);
    project.model() = std::move(loaded_3mf.model);

    for (const Loaded3MF::ConfigContainerData& cc_data : loaded_3mf.config_containers_data) {
        project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
        auto& mutable_selected_preset = project.config_containers().back()->mutable_selected_preset();
        mutable_selected_preset = Domain::Preset::SelectedPreset::make(
            cc_data.preset,
            cc_data.config_pack
        );
    }

    if (project.config_containers().empty()) {
        auto cc = std::make_unique<Domain::ConfigContainer>();

        project.config_containers().emplace_back(std::move(cc));
    }
    return project;
}



static Loaded3MF load_legacy_project(const std::string& file_path)
{
    Loaded3MF loaded_3mf;
    loaded_3mf.config_containers_data.emplace_back();
    Domain::Model model;
    

    Domain::WipeTowersOnBeds wipe_towers;
    Domain::CustomGCodesOnBeds custom_gcodes;
    std::optional<Semver> version;

    if (! Slic3rLegacy::load_3mf_legacy(file_path.c_str(), loaded_3mf.config_containers_data.front().config_pack, &loaded_3mf.model,
        false, loaded_3mf.version, wipe_towers, custom_gcodes))
        throw Loaded3MFException(Read3mfIssue(Read3mfIssueType::legacy_loader_failed, "Loading of legacy 3MF failed."));

    loaded_3mf.filepath_3mf = file_path;
    return loaded_3mf;
}

void load_project(
    const std::string& file_path,
    std::function<void(Domain::Project&&)> after_load,
    std::function<void(std::exception_ptr)> after_exception)
{
    Platform::PlatformServices::instance().job_manager().create_job("project_load",
        [](Biz::JThread::StopToken stop_token, const std::string file_path) -> Domain::Project {
            try {
                Loaded3MF loaded_3mf = load_3mf(file_path);
                // TODO: Loaded3MF contains list of issue encountered when loading.
                // In would make sense to let it propagate somewhere.
                return convert_to_project(std::move(loaded_3mf));
            }
            catch (const Loaded3MFException& e) {
                if (e.issue.type != Read3mfIssueType::legacy_loader_required)
                    throw;
            }
            // In this case, we can try to use the legacy loader (project from PrusaSlicer <3.0.0).
            // The old loader only loads the project, no list of issues is collected when the load
            // is sucessful. Otherwise the function throws - which is now ignored.
            return convert_to_project(load_legacy_project(file_path));
        },
        file_path
    ).on_result(after_load).on_exception(after_exception).start();
}

} // namespace Slic3r::Biz
