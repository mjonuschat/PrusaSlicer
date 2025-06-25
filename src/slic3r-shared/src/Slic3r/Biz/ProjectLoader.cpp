#include "Slic3r/Biz/ProjectLoader.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Biz/Platform/JobManager/JobManager.hpp"

namespace Slic3r::Biz {

static Domain::Project load_legacy_project(const std::string& file_path)
{
    Domain::Project project;

    Domain::ConfigPack config;
    Domain::WipeTowersOnBeds wipe_towers;
    Domain::CustomGCodesOnBeds custom_gcodes;
    boost::optional<Semver> version;

    if (! Slic3rLegacy::load_3mf_legacy(file_path.c_str(), config, &project.model(),
        false, version, wipe_towers, custom_gcodes))
        throw Loaded3MFException(Read3mfIssue(Read3mfIssueType::legacy_loader_failed, "Loading of legacy 3MF failed."));

    project.set_file_name(file_path);
    // TODO: implement
    project.config_containers().clear();
    project.config_containers().emplace_back(std::make_unique<Domain::ConfigContainer>());
    auto& config_container = project.config_containers().back();
    config_container->set_print_config_new(config);
    DynamicPrintConfig co;
    auto full{FullPrintConfig::defaults()};
    co.apply(full);
    config_container->set_print_config(co);
    return project;
}

void load_project(const std::string& file_path, std::function<void(Domain::Project&&)> after_load)
{
    Platform::PlatformServices::instance().job_manager().create_job("project_load",
        [](Biz::JThread::StopToken stop_token, const std::string file_path) -> Domain::Project {
            try {
                Loaded3MF loaded_3mf = load_3mf(file_path);
                // TODO: Loaded3MF contains list of issue encountered when loading.
                // In would make sense to let it propagate somewhere.
                Domain::Project project = std::move(loaded_3mf.project);
                return project;
            }
            catch (const Loaded3MFException& e) {
                if (e.issue.type != Read3mfIssueType::legacy_loader_required)
                    throw;
            }
            // In this case, we can try to use the legacy loader (project from PrusaSlicer <3.0.0).
            // The old loader only loads the project, no list of issues is collected when the load
            // is sucessful. Otherwisem the function throws - which is now ignored.
            return load_legacy_project(file_path);
        },
        file_path
    ).on_result(after_load).start();
}

} // namespace Slic3r::Biz
