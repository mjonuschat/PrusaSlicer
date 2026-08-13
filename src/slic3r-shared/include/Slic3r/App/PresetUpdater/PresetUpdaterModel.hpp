#pragma once

#include "Slic3r/App/PresetUpdater/PresetUpdaterActivityReporter.hpp"
#include "Slic3r/Biz/ObservableList.hpp"
#include "Slic3r/Biz/PresetUpdater/IPresetUpdaterResultListener.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "Slic3r/Biz/Platform/TimerQueue.hpp"
#include "Slic3r/Semver.hpp"

#include <boost/filesystem/path.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Slic3r::App {

struct PresetUpdaterVendorRowState
{
    enum class InstallState
    {
        Idle,
        Queued,
        Running,
        Done,
        Failed
    };

    std::string repo_id;
    std::string vendor_id;
    std::string comment;
    Biz::PresetUpdater::VendorReconfigurationState state{
        Biz::PresetUpdater::VendorReconfigurationState::Update
    };
    Slic3r::Semver current_version;
    Slic3r::Semver recommended_version;

    InstallState install_state{InstallState::Idle};
    std::string error_text;

    /// Set while a forced reconfiguration is pending: only the whole of it may be installed.
    bool install_locked{false};

    bool skipped{false};
};

using PresetUpdaterVendorList = Biz::ObservableList<PresetUpdaterVendorRowState>;

struct PresetUpdaterSourceCounts
{
    int updates{0};
    int new_vendors{0};
    int required{0}; ///< Installed profiles too old or too new to be used as they are.
    int unknown_versions{0}; ///< Installed profiles of a version the source does not list.

    int pending() const
    {
        return updates + new_vendors + required + unknown_versions;
    }

    /// What cannot be left alone, because those profiles are unusable until they are changed.
    int attention() const
    {
        return required + unknown_versions;
    }
};

struct PresetUpdaterSourceRowState
{
    enum class UpdateState
    {
        NotChecked,
        Waiting, ///< Selected, its check job is queued.
        Checking,
        Installing,
        UpToDate,
        HasUpdates
    };

    std::string uuid;
    std::string id;
    std::string name;
    std::string description;
    std::string visibility;
    boost::filesystem::path zip_path; ///< Non-empty for a local (offline) source.

    bool selected{false};
    /// Held still while an install is in flight for a source sharing this one's id.
    bool selection_locked{false};
    /// Set while a forced reconfiguration is pending: only the whole of it may be installed.
    bool install_locked{false};
    UpdateState update_state{UpdateState::NotChecked};
    PresetUpdaterSourceCounts counts;
    /// Vendors the check could not evaluate, deduplicated and deliberately outside the counts.
    std::vector<std::string> skipped_vendors;
    /// The check failed before reading anything, which is not the same as having no updates.
    bool check_failed{false};

    /// Shared on purpose: every copy of this state must bind to the same vendor list.
    std::shared_ptr<PresetUpdaterVendorList> vendors{std::make_shared<PresetUpdaterVendorList>()};
};

using PresetUpdaterSourceList = Biz::ObservableList<PresetUpdaterSourceRowState>;

class IPresetUpdaterModelListener
{
public:
    virtual ~IPresetUpdaterModelListener() = default;

    virtual void on_preset_updater_model_changed() {}
};

/// State behind PresetUpdaterDialog, shared by every instance of it.
class PresetUpdaterModel :
    public Biz::PresetUpdater::IPresetUpdaterResultListener,
    public WithListeners<IPresetUpdaterModelListener>
{
public:
    enum class Status
    {
        Idle,
        ListingSources,
        Checking,
        Installing,
        UpToDate,
        Warned
    };

    explicit PresetUpdaterModel(
        Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor
    );

    ~PresetUpdaterModel() override;

    /// Must be called before the interactor goes; the model stops doing anything afterwards.
    void shutdown();

    void on_dialog_opened();
    void on_dialog_closed();

    bool dialog_open() const;

    void set_source_selected(const std::string& uuid, bool selected);
    void add_local_repository(const boost::filesystem::path& zip_path);
    void remove_local_repository(const std::string& uuid);

    void update_vendor(const std::string& repo_id, const std::string& vendor_id);
    void update_source(const std::string& uuid);
    void update_everything();

    void update_required();

    /// Starts the work a launch owes. Called once the render modules and callbacks are in place.
    void start();

    void set_show_dialog_callback(std::function<void()> callback);

    void set_presets_installed_callback(std::function<void()> callback);

    /// Called once the check started by start() is over, whether it delivered a list or failed.
    void set_forced_check_finished_callback(std::function<void(bool has_forced)> callback);

    bool has_actionable_updates() const;

    /// Whether anything is left of what a forced reconfiguration requires.
    bool has_required_updates() const;

    bool has_forced_reconfigurations() const;

    /**
     * @brief Whether the preset updater may use the network at all.
     *
     * When false, sources are listed from the data dir, online sources are never contacted, and
     * only what comes with the installation and what local sources offer can be installed.
     */
    bool online_allowed() const;

    Status status() const;

    PresetUpdaterSourceList& online_sources();
    PresetUpdaterSourceList& local_sources();

    void on_preset_updater_error(
        Biz::PresetUpdater::JobId job_id,
        const std::string& body,
        Biz::PresetUpdater::PresetUpdaterReason reason
    ) override;
    void on_preset_updater_forced_reconfigurations_list(
        Biz::PresetUpdater::JobId job_id,
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_reconfigurations_list(
        Biz::PresetUpdater::JobId job_id,
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings,
        Biz::PresetUpdater::VerboseStyle verbose
    ) override;
    void on_preset_updater_reconfigurations_performed(
        Biz::PresetUpdater::JobId job_id,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_status(
        Biz::PresetUpdater::JobId job_id,
        const std::string& target,
        int attempt,
        unsigned delay,
        Biz::PresetUpdater::VerboseStyle verbose
    ) override;
    void on_preset_updater_repository_info_vector(
        Biz::PresetUpdater::JobId job_id,
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_repository_selection_performed(
        Biz::PresetUpdater::JobId job_id,
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
        const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
    ) override;
    void on_preset_updater_job_finished(
        Biz::PresetUpdater::JobId job_id, Biz::PresetUpdater::JobState state
    ) override;

private:
    struct VendorKey
    {
        std::string repo_id;
        std::string vendor_id;
    };

    struct InstallJob
    {
        std::vector<VendorKey> keys;
        std::vector<PresetUpdaterActivityReporter::InstalledVendor> installed;
    };

    using SourceMutator = std::function<void(PresetUpdaterSourceRowState&)>;
    using VendorMutator = std::function<void(PresetUpdaterVendorRowState&)>;

    void start_listing();
    void start_check();
    Biz::PresetUpdater::JobId start_install(const std::vector<VendorKey>& keys);

    void cancel_pending_check();

    void schedule_selection_commit();

    void commit_selection();

    /// The application only draws on demand, so a change made outside a frame must ask for one.
    static void request_render();

    void set_status(Status status);
    void refresh_status();
    void notify_changed();

    bool needs_check() const;

    bool has_failures() const;

    void rebuild_sources(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor
    );
    void mutate_source(const std::string& uuid, const SourceMutator& mutator);
    void mutate_source_by_repo_id(const std::string& repo_id, const SourceMutator& mutator);
    void mutate_all_sources(const SourceMutator& mutator);
    void mutate_vendor(const VendorKey& key, const VendorMutator& mutator);
    void refresh_source_counts(PresetUpdaterSourceRowState& source);

    void refresh_locks();

    bool selection_locked(const std::string& uuid) const;

    bool has_source_with_id(const std::string& id) const;

    bool has_selected_source_with_id(const std::string& id) const;

    void set_problem_warnings(std::vector<Biz::PresetUpdater::PresetUpdaterWarning> warnings);

    std::string source_name_of_uuid(const std::string& uuid) const;

    std::vector<PresetUpdaterActivityReporter::Problem> current_problems() const;

    /// Folds a fresh check result into one source's vendor rows instead of replacing them.
    void merge_vendor_rows(
        PresetUpdaterSourceRowState& source,
        const std::vector<PresetUpdaterVendorRowState>& incoming
    );

    /// @param required_only Keep only the vendors whose profiles are unusable as they are.
    std::vector<VendorKey> actionable_keys_of_source(
        const PresetUpdaterSourceRowState& source, bool required_only = false
    ) const;

    void update_forced_vendors(
        const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations
    );

    void show_dialog();

    Biz::PresetUpdater::PresetUpdaterInteractor& m_preset_updater_interactor;
    PresetUpdaterActivityReporter m_activity_reporter;

    PresetUpdaterSourceList m_online_sources;
    PresetUpdaterSourceList m_local_sources;
    Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector m_working_selection;

    Status m_status{Status::Idle};
    bool m_operation_failed{false};
    std::vector<Biz::PresetUpdater::PresetUpdaterWarning> m_problem_warnings;

    std::map<Biz::PresetUpdater::JobId, std::string> m_job_subjects;

    std::function<void()> m_show_dialog_callback;
    std::function<void()> m_presets_installed_callback;
    std::function<void(bool has_forced)> m_forced_check_finished_callback;

    std::vector<VendorKey> m_forced_vendors;

    size_t m_open_count{0};
    Biz::PresetUpdater::JobId m_check_job{Biz::PresetUpdater::k_invalid_job_id};
    Biz::PresetUpdater::JobId m_forced_check_job{Biz::PresetUpdater::k_invalid_job_id};
    std::map<Biz::PresetUpdater::JobId, InstallJob> m_install_jobs;

    Biz::PresetUpdater::JobId m_selection_job{Biz::PresetUpdater::k_invalid_job_id};
    bool m_commit_dirty{false};
    /// Superseded, never cancelled: TimerQueue::cancel_timer aborts on an already dispatched timer.
    uint64_t m_commit_generation{0};
    bool m_commit_armed{false};

    /// Set by shutdown(). The interactor reference is not to be used past this point.
    bool m_shut_down{false};
};

} // namespace Slic3r::App
