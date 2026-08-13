#include "Slic3r/App/PresetUpdater/PresetUpdaterModel.hpp"

#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/AppServices.hpp"

#include "Slic3r/Biz/Platform/PlatformServices.hpp"
#include "Slic3r/Log.hpp"

#include <algorithm>
#include <chrono>
#include <set>

namespace Slic3r::App {

namespace {

constexpr std::chrono::milliseconds k_selection_commit_debounce{300};

using Activity = PresetUpdaterActivityReporter::Activity;
using Biz::PresetUpdater::VendorReconfigurationState;

std::vector<Biz::PresetUpdater::VendorReconfiguration> all_reconfigurations(
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& list
)
{
    std::vector<Biz::PresetUpdater::VendorReconfiguration> result;
    for (const std::vector<Biz::PresetUpdater::VendorReconfiguration>* bucket :
         {&list.regular_updates(),
          &list.forced_updates(),
          &list.forced_downgrades(),
          &list.not_in_index(),
          &list.new_vendors()})
    {
        result.insert(result.end(), bucket->begin(), bucket->end());
    }
    return result;
}

void log_reconfigurations(const Biz::PresetUpdater::PresetUpdaterReconfigurationList& list)
{
    const std::pair<const char*, const std::vector<Biz::PresetUpdater::VendorReconfiguration>*>
        buckets[]{
            {"update", &list.regular_updates()},
            {"new vendor", &list.new_vendors()},
            {"not in index", &list.not_in_index()},
            {"forced update", &list.forced_updates()},
            {"forced downgrade", &list.forced_downgrades()},
        };
    for (const auto& [name, bucket] : buckets) {
        for (const Biz::PresetUpdater::VendorReconfiguration& reconfiguration : *bucket) {
            SPDLOG_INFO(
                "{}: {}/{}", name, reconfiguration.vendor_repo_id, reconfiguration.vendor_id
            );
        }
    }
}

void log_warnings(const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings)
{
    for (const Biz::PresetUpdater::PresetUpdaterWarning& warning : warnings) {
        SPDLOG_WARN("Preset updater warning: {}", warning.string());
    }
}

bool is_installing(const PresetUpdaterSourceRowState& source)
{
    for (size_t index = 0; index < source.vendors->size(); ++index) {
        const PresetUpdaterVendorRowState::InstallState install_state =
            source.vendors->at(index).install_state;
        if (install_state == PresetUpdaterVendorRowState::InstallState::Queued
            || install_state == PresetUpdaterVendorRowState::InstallState::Running)
        {
            return true;
        }
    }
    return false;
}

template <class Data, class Mutator>
void mutate_element(Biz::ObservableList<Data>& list, size_t index, const Mutator& mutator)
{
    Data element{list.at(index)};
    mutator(element);
    list.set(std::move(element), index);
}

} // namespace

PresetUpdaterModel::PresetUpdaterModel(
    Biz::PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor
) :
    m_preset_updater_interactor(preset_updater_interactor),
    m_activity_reporter(preset_updater_interactor)
{
    m_preset_updater_interactor.add_listener<Biz::PresetUpdater::IPresetUpdaterResultListener>(this);
}

void PresetUpdaterModel::start()
{
    if (m_shut_down) {
        return;
    }
    m_forced_check_job = m_preset_updater_interactor.check_forced_reconfigurations();
}

void PresetUpdaterModel::set_show_dialog_callback(std::function<void()> callback)
{
    m_show_dialog_callback = callback;
    m_activity_reporter.set_show_dialog_callback(std::move(callback));
}

void PresetUpdaterModel::set_presets_installed_callback(std::function<void()> callback)
{
    m_presets_installed_callback = std::move(callback);
}

void PresetUpdaterModel::set_forced_check_finished_callback(
    std::function<void(bool has_forced)> callback
)
{
    m_forced_check_finished_callback = std::move(callback);
}

bool PresetUpdaterModel::has_forced_reconfigurations() const
{
    if (m_forced_vendors.empty()) {
        return false;
    }

    if (m_online_sources.size() == 0 && m_local_sources.size() == 0) {
        return true;
    }

    for (const VendorKey& forced : m_forced_vendors) {
        if (has_selected_source_with_id(forced.repo_id)) {
            return true;
        }
    }
    return false;
}

void PresetUpdaterModel::update_forced_vendors(
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations
)
{
    m_forced_vendors.clear();
    for (const std::vector<Biz::PresetUpdater::VendorReconfiguration>* bucket :
         {&reconfigurations.forced_updates(), &reconfigurations.forced_downgrades()})
    {
        for (const Biz::PresetUpdater::VendorReconfiguration& reconfiguration : *bucket) {
            m_forced_vendors.push_back(
                VendorKey{reconfiguration.vendor_repo_id, reconfiguration.vendor_id}
            );
        }
    }
}

void PresetUpdaterModel::show_dialog()
{
    if (m_shut_down || dialog_open() || !m_show_dialog_callback) {
        return;
    }
    m_show_dialog_callback();
}

PresetUpdaterModel::~PresetUpdaterModel() = default;

void PresetUpdaterModel::shutdown()
{
    if (m_shut_down) {
        return;
    }
    m_shut_down = true;

    ++m_commit_generation;
    m_commit_armed = false;
    m_commit_dirty = false;

    m_activity_reporter.reset();
    set_show_dialog_callback(nullptr);
    m_presets_installed_callback     = nullptr;
    m_forced_check_finished_callback = nullptr;
    m_forced_vendors.clear();

    m_preset_updater_interactor.remove_listener<Biz::PresetUpdater::IPresetUpdaterResultListener>(
        this
    );
}

bool PresetUpdaterModel::online_allowed() const
{
    return AppServices::instance().app_config().get<bool>("enable_preset_update");
}

void PresetUpdaterModel::on_dialog_opened()
{
    if (m_shut_down) {
        return;
    }
    m_activity_reporter.set_dialog_open(true);

    ++m_open_count;
    if (m_open_count == 1) {
        start_listing();
    }
}

void PresetUpdaterModel::on_dialog_closed()
{
    if (m_open_count > 0) {
        --m_open_count;
    }
    if (m_open_count > 0 || m_shut_down) {
        return;
    }

    m_activity_reporter.set_dialog_open(false);

    if (m_commit_armed) {
        ++m_commit_generation;
        m_commit_armed = false;
        commit_selection();
    }
}

bool PresetUpdaterModel::dialog_open() const
{
    return m_open_count > 0;
}

PresetUpdaterModel::Status PresetUpdaterModel::status() const
{
    return m_status;
}

PresetUpdaterSourceList& PresetUpdaterModel::online_sources()
{
    return m_online_sources;
}

PresetUpdaterSourceList& PresetUpdaterModel::local_sources()
{
    return m_local_sources;
}

void PresetUpdaterModel::set_status(Status status)
{
    m_status = status;
    notify_changed();
}

void PresetUpdaterModel::refresh_status()
{
    if (has_failures()) {
        set_status(Status::Warned);
    } else if (has_actionable_updates()) {
        set_status(Status::Idle);
    } else {
        set_status(Status::UpToDate);
    }
}

bool PresetUpdaterModel::needs_check() const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            const PresetUpdaterSourceRowState& source = list->at(index);
            if (!source.selected) {
                continue;
            }
            // Checking counts as unknown: a cancelled check leaves that state behind.
            if (source.update_state == PresetUpdaterSourceRowState::UpdateState::NotChecked
                || source.update_state == PresetUpdaterSourceRowState::UpdateState::Waiting
                || source.update_state == PresetUpdaterSourceRowState::UpdateState::Checking)
            {
                return true;
            }
        }
    }
    return false;
}

bool PresetUpdaterModel::has_failures() const
{
    if (m_operation_failed) {
        return true;
    }
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            const PresetUpdaterSourceRowState& source = list->at(index);
            if (source.selected && source.check_failed) {
                return true;
            }
        }
    }
    return false;
}

void PresetUpdaterModel::request_render()
{
    Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
}

void PresetUpdaterModel::notify_changed()
{
    refresh_locks();
    m_activity_reporter.report_problems(current_problems());
    invoke_listeners<IPresetUpdaterModelListener>([](IPresetUpdaterModelListener* listener)
                                                  { listener->on_preset_updater_model_changed(); });
    request_render();
}

void PresetUpdaterModel::refresh_locks()
{
    const bool install_locked = has_forced_reconfigurations();

    std::set<std::string> locked_ids;
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (is_installing(list->at(index))) {
                locked_ids.insert(list->at(index).id);
            }
        }
    }

    for (const VendorKey& forced : m_forced_vendors) {
        if (has_selected_source_with_id(forced.repo_id)) {
            locked_ids.insert(forced.repo_id);
        }
    }

    for (PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            const bool locked = locked_ids.contains(list->at(index).id);
            if (list->at(index).selection_locked != locked
                || list->at(index).install_locked != install_locked)
            {
                mutate_element(
                    *list,
                    index,
                    [locked, install_locked](PresetUpdaterSourceRowState& source)
                    {
                        source.selection_locked = locked;
                        source.install_locked   = install_locked;
                    }
                );
            }

            PresetUpdaterVendorList& vendors = *list->at(index).vendors;
            for (size_t vendor_index = 0; vendor_index < vendors.size(); ++vendor_index) {
                if (vendors.at(vendor_index).install_locked == install_locked) {
                    continue;
                }
                mutate_element(
                    vendors,
                    vendor_index,
                    [install_locked](PresetUpdaterVendorRowState& vendor)
                    { vendor.install_locked = install_locked; }
                );
            }
        }
    }
}

bool PresetUpdaterModel::has_source_with_id(const std::string& id) const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (list->at(index).id == id) {
                return true;
            }
        }
    }
    return false;
}

bool PresetUpdaterModel::has_selected_source_with_id(const std::string& id) const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            const PresetUpdaterSourceRowState& source = list->at(index);
            if (source.id == id && source.selected) {
                return true;
            }
        }
    }
    return false;
}

void PresetUpdaterModel::set_problem_warnings(
    std::vector<Biz::PresetUpdater::PresetUpdaterWarning> warnings
)
{
    m_problem_warnings = std::move(warnings);
    m_activity_reporter.report_problems(current_problems());
}

std::vector<PresetUpdaterActivityReporter::Problem> PresetUpdaterModel::current_problems() const
{
    std::vector<PresetUpdaterActivityReporter::Problem> problems;
    for (const Biz::PresetUpdater::PresetUpdaterWarning& warning : m_problem_warnings) {
        // The repo id is all a warning carries, and all there is to go on until a listing lands.
        std::string source_name = warning.repo;
        bool dropped            = false;
        for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
            for (size_t index = 0; index < list->size(); ++index) {
                const PresetUpdaterSourceRowState& source = list->at(index);
                if (source.id != warning.repo) {
                    continue;
                }
                dropped     = !source.selected;
                source_name = source.name;
            }
        }
        if (dropped) {
            continue;
        }
        problems.push_back(
            PresetUpdaterActivityReporter::Problem{warning.reason, source_name, warning.vendor}
        );
    }
    return problems;
}

bool PresetUpdaterModel::selection_locked(const std::string& uuid) const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (list->at(index).uuid == uuid) {
                return list->at(index).selection_locked;
            }
        }
    }
    return false;
}

void PresetUpdaterModel::start_listing()
{
    cancel_pending_check();

    // A reopen is not a fresh start: an install in flight still owns its rows and its job.
    m_operation_failed = false;
    ++m_commit_generation;
    m_commit_armed = false;

    set_status(Status::ListingSources);
    m_activity_reporter.begin_activity(
        m_preset_updater_interactor.list_repositories(online_allowed()), Activity::ListingSources
    );
}

void PresetUpdaterModel::start_check()
{
    m_problem_warnings.clear();
    m_operation_failed = false;

    mutate_all_sources(
        [](PresetUpdaterSourceRowState& source)
        {
            if (!source.selected) {
                source.update_state = PresetUpdaterSourceRowState::UpdateState::NotChecked;
                return;
            }
            // A source that already has a result keeps showing it until the new one replaces it.
            if (source.update_state == PresetUpdaterSourceRowState::UpdateState::NotChecked
                || source.update_state == PresetUpdaterSourceRowState::UpdateState::Waiting)
            {
                source.update_state = PresetUpdaterSourceRowState::UpdateState::Checking;
            }
        }
    );

    set_status(Status::Checking);
    m_check_job = m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(
        online_allowed(),
        Biz::PresetUpdater::VerboseStyle::NoProgress,
        false,
        Biz::PresetUpdater::SourceListSync::UseStored
    );
    m_activity_reporter.begin_activity(m_check_job, Activity::Checking);
}

Biz::PresetUpdater::JobId PresetUpdaterModel::start_install(const std::vector<VendorKey>& keys)
{
    if (keys.empty()) {
        return Biz::PresetUpdater::k_invalid_job_id;
    }

    m_operation_failed = false;

    Biz::PresetUpdater::PresetUpdaterReconfigurationList list;
    std::vector<PresetUpdaterActivityReporter::InstalledVendor> installed;
    for (const VendorKey& key : keys) {
        mutate_vendor(
            key,
            [&list, &installed](PresetUpdaterVendorRowState& vendor)
            {
                list.emplace_back(
                    vendor.state,
                    vendor.vendor_id,
                    vendor.repo_id,
                    vendor.current_version,
                    vendor.recommended_version,
                    vendor.comment
                );
                installed.push_back(
                    PresetUpdaterActivityReporter::InstalledVendor{
                        vendor.vendor_id,
                        vendor.current_version,
                        vendor.recommended_version,
                        vendor.state
                    }
                );
            }
        );
    }

    const Biz::PresetUpdater::JobId job_id = m_preset_updater_interactor.perform_reconfigurations(
        list,
        Biz::PresetUpdater::ReconfigurationType::All
    );
    m_install_jobs[job_id] = InstallJob{keys, std::move(installed)};

    // The interactor runs one job at a time, so anything behind another one has not started yet.
    const PresetUpdaterVendorRowState::InstallState initial_install_state =
        m_preset_updater_interactor.pending_job_count() > 1 ?
        PresetUpdaterVendorRowState::InstallState::Queued :
        PresetUpdaterVendorRowState::InstallState::Running;

    for (const VendorKey& key : keys) {
        mutate_vendor(
            key,
            [initial_install_state](PresetUpdaterVendorRowState& vendor)
            {
                vendor.install_state = initial_install_state;
                vendor.error_text.clear();
            }
        );
    }

    set_status(Status::Installing);
    m_activity_reporter.begin_activity(job_id, Activity::Installing);
    return job_id;
}

void PresetUpdaterModel::cancel_pending_check()
{
    if (m_check_job == Biz::PresetUpdater::k_invalid_job_id) {
        return;
    }
    if (m_preset_updater_interactor.is_job_pending(m_check_job)) {
        m_preset_updater_interactor.cancel_job(m_check_job);
    }
}

void PresetUpdaterModel::schedule_selection_commit()
{
    m_commit_armed              = true;
    const uint64_t generation   = ++m_commit_generation;

    Biz::Platform::PlatformServices::instance().timer_queue().set_timer(
        k_selection_commit_debounce,
        [this, generation]()
        {
            if (generation != m_commit_generation) {
                return;
            }
            m_commit_armed = false;
            commit_selection();
        }
    );
}

void PresetUpdaterModel::commit_selection()
{
    if (m_shut_down) {
        return;
    }

    if (m_selection_job != Biz::PresetUpdater::k_invalid_job_id
        && m_preset_updater_interactor.is_job_pending(m_selection_job))
    {
        m_commit_dirty = true;
        return;
    }

    m_commit_dirty     = false;
    m_operation_failed = false;
    m_selection_job = m_preset_updater_interactor.apply_repository_selection(m_working_selection);
    m_activity_reporter.begin_activity(m_selection_job, Activity::ApplyingSelection);
}

void PresetUpdaterModel::set_source_selected(const std::string& uuid, bool selected)
{
    if (selection_locked(uuid)) {
        return;
    }

    std::string selected_id;
    for (Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info : m_working_selection) {
        if (info.descriptor.uuid == uuid) {
            selected_id = info.descriptor.id;
            break;
        }
    }

    for (Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info : m_working_selection) {
        if (info.descriptor.uuid == uuid) {
            info.selected = selected;
        } else if (selected && info.descriptor.id == selected_id) {
            // Only one source per id may be selected, apply_selection treats a clash as fatal.
            info.selected = false;
        }
    }

    mutate_all_sources(
        [&uuid, &selected_id, selected](PresetUpdaterSourceRowState& source)
        {
            if (source.uuid == uuid) {
                source.selected = selected;
            } else if (selected && source.id == selected_id) {
                source.selected = false;
            }

            if (!source.selected) {
                source.update_state = PresetUpdaterSourceRowState::UpdateState::NotChecked;
                source.vendors->reset(std::vector<PresetUpdaterVendorRowState>{});
                source.counts = PresetUpdaterSourceCounts{};
                source.skipped_vendors.clear();
                source.check_failed = false;
            } else if (source.update_state
                       == PresetUpdaterSourceRowState::UpdateState::NotChecked)
            {
                source.update_state = PresetUpdaterSourceRowState::UpdateState::Waiting;
            }
        }
    );

    cancel_pending_check();
    schedule_selection_commit();
    notify_changed();
}

void PresetUpdaterModel::add_local_repository(const boost::filesystem::path& zip_path)
{
    cancel_pending_check();
    m_operation_failed = false;
    set_status(Status::ListingSources);
    const Biz::PresetUpdater::JobId job_id =
        m_preset_updater_interactor.add_local_repository(zip_path);
    m_job_subjects[job_id] = zip_path.filename().string();
    m_activity_reporter.begin_activity(job_id, Activity::AddingSource);
}

void PresetUpdaterModel::remove_local_repository(const std::string& uuid)
{
    cancel_pending_check();
    m_operation_failed = false;
    set_status(Status::ListingSources);
    const Biz::PresetUpdater::JobId job_id =
        m_preset_updater_interactor.remove_local_repository(uuid);
    m_job_subjects[job_id] = source_name_of_uuid(uuid);
    m_activity_reporter.begin_activity(job_id, Activity::RemovingSource);
}

std::string PresetUpdaterModel::source_name_of_uuid(const std::string& uuid) const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (list->at(index).uuid == uuid) {
                return list->at(index).name;
            }
        }
    }
    return {};
}

void PresetUpdaterModel::update_vendor(const std::string& repo_id, const std::string& vendor_id)
{
    start_install({VendorKey{repo_id, vendor_id}});
}

void PresetUpdaterModel::update_source(const std::string& uuid)
{
    std::vector<VendorKey> keys;
    mutate_source(
        uuid,
        [this, &keys](PresetUpdaterSourceRowState& source)
        { keys = actionable_keys_of_source(source); }
    );
    start_install(keys);
}

void PresetUpdaterModel::update_everything()
{
    std::vector<VendorKey> keys;
    mutate_all_sources(
        [this, &keys](PresetUpdaterSourceRowState& source)
        {
            const std::vector<VendorKey> source_keys = actionable_keys_of_source(source);
            keys.insert(keys.end(), source_keys.begin(), source_keys.end());
        }
    );
    start_install(keys);
}

void PresetUpdaterModel::update_required()
{
    std::vector<VendorKey> keys;
    mutate_all_sources(
        [this, &keys](PresetUpdaterSourceRowState& source)
        {
            const std::vector<VendorKey> source_keys = actionable_keys_of_source(source, true);
            keys.insert(keys.end(), source_keys.begin(), source_keys.end());
        }
    );
    start_install(keys);
}

std::vector<PresetUpdaterModel::VendorKey> PresetUpdaterModel::actionable_keys_of_source(
    const PresetUpdaterSourceRowState& source, bool required_only
) const
{
    std::vector<VendorKey> keys;
    for (size_t index = 0; index < source.vendors->size(); ++index) {
        const PresetUpdaterVendorRowState& vendor = source.vendors->at(index);
        // Idle only because nothing was ever started for it, and there is nothing to install.
        if (vendor.skipped) {
            continue;
        }
        if (required_only && vendor.state != VendorReconfigurationState::ForcedUpdate
            && vendor.state != VendorReconfigurationState::ForcedDowngrade)
        {
            continue;
        }
        if (vendor.install_state == PresetUpdaterVendorRowState::InstallState::Idle
            || vendor.install_state == PresetUpdaterVendorRowState::InstallState::Failed)
        {
            keys.push_back(VendorKey{vendor.repo_id, vendor.vendor_id});
        }
    }
    return keys;
}

bool PresetUpdaterModel::has_actionable_updates() const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (!actionable_keys_of_source(list->at(index)).empty()) {
                return true;
            }
        }
    }
    return false;
}

bool PresetUpdaterModel::has_required_updates() const
{
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (!actionable_keys_of_source(list->at(index), true).empty()) {
                return true;
            }
        }
    }
    return false;
}

void PresetUpdaterModel::rebuild_sources(
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor
)
{
    m_working_selection = descriptor;

    std::map<std::string, PresetUpdaterSourceRowState> previous;
    for (const PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            previous.emplace(list->at(index).uuid, list->at(index));
        }
    }

    std::vector<PresetUpdaterSourceRowState> online;
    std::vector<PresetUpdaterSourceRowState> local;

    for (const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info : descriptor) {
        PresetUpdaterSourceRowState source;
        source.uuid        = info.descriptor.uuid;
        source.id          = info.descriptor.id;
        source.name        = info.descriptor.name;
        source.description = info.descriptor.description;
        source.visibility  = info.descriptor.visibility;
        source.zip_path    = info.descriptor.zip_path;
        source.selected    = info.selected;
        source.update_state =
            info.selected ? PresetUpdaterSourceRowState::UpdateState::Waiting :
                            PresetUpdaterSourceRowState::UpdateState::NotChecked;

        const auto previous_source = previous.find(source.uuid);
        if (previous_source != previous.end() && source.selected) {
            source.vendors         = previous_source->second.vendors;
            source.update_state    = previous_source->second.update_state;
            source.counts          = previous_source->second.counts;
            source.skipped_vendors = previous_source->second.skipped_vendors;
            source.check_failed    = previous_source->second.check_failed;
        }

        if (source.zip_path.empty()) {
            online.push_back(std::move(source));
        } else {
            local.push_back(std::move(source));
        }
    }

    m_online_sources.reset(std::move(online));
    m_local_sources.reset(std::move(local));
}

void PresetUpdaterModel::mutate_source(const std::string& uuid, const SourceMutator& mutator)
{
    for (PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (list->at(index).uuid != uuid) {
                continue;
            }
            mutate_element(*list, index, mutator);
            return;
        }
    }
}

void PresetUpdaterModel::mutate_source_by_repo_id(
    const std::string& repo_id, const SourceMutator& mutator
)
{
    for (PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            if (list->at(index).id != repo_id || !list->at(index).selected) {
                continue;
            }
            mutate_element(*list, index, mutator);
            return;
        }
    }
}

void PresetUpdaterModel::mutate_all_sources(const SourceMutator& mutator)
{
    for (PresetUpdaterSourceList* list : {&m_online_sources, &m_local_sources}) {
        for (size_t index = 0; index < list->size(); ++index) {
            mutate_element(*list, index, mutator);
        }
    }
}

void PresetUpdaterModel::mutate_vendor(const VendorKey& key, const VendorMutator& mutator)
{
    mutate_source_by_repo_id(
        key.repo_id,
        [&key, &mutator, this](PresetUpdaterSourceRowState& source)
        {
            for (size_t index = 0; index < source.vendors->size(); ++index) {
                if (source.vendors->at(index).vendor_id != key.vendor_id) {
                    continue;
                }
                mutate_element(*source.vendors, index, mutator);
                break;
            }
            refresh_source_counts(source);
        }
    );
}

void PresetUpdaterModel::merge_vendor_rows(
    PresetUpdaterSourceRowState& source,
    const std::vector<PresetUpdaterVendorRowState>& incoming
)
{
    std::map<std::string, PresetUpdaterVendorRowState> previous;
    for (size_t index = 0; index < source.vendors->size(); ++index) {
        const PresetUpdaterVendorRowState& row = source.vendors->at(index);
        previous.emplace(row.vendor_id, row);
    }

    std::vector<PresetUpdaterVendorRowState> merged;
    merged.reserve(incoming.size() + previous.size());

    for (const PresetUpdaterVendorRowState& vendor : incoming) {
        PresetUpdaterVendorRowState row = vendor;
        const auto previous_row         = previous.find(vendor.vendor_id);
        if (previous_row != previous.end()
            && previous_row->second.install_state
                   != PresetUpdaterVendorRowState::InstallState::Done)
        {
            // Work in flight, or a failure the user has not dealt with, outlives a re-check.
            row.install_state = previous_row->second.install_state;
            row.error_text    = previous_row->second.error_text;
        }
        merged.push_back(std::move(row));
        previous.erase(vendor.vendor_id);
    }

    // What is left is no longer offered. Keeping the installed ones leaves their confirmation up.
    for (const auto& [vendor_id, row] : previous) {
        if (row.install_state == PresetUpdaterVendorRowState::InstallState::Done) {
            merged.push_back(row);
        }
    }

    source.vendors->reset(std::move(merged));
}

void PresetUpdaterModel::refresh_source_counts(PresetUpdaterSourceRowState& source)
{
    source.counts   = PresetUpdaterSourceCounts{};
    bool installing = false;
    for (size_t index = 0; index < source.vendors->size(); ++index) {
        const PresetUpdaterVendorRowState& vendor = source.vendors->at(index);
        if (vendor.skipped) {
            continue;
        }
        if (vendor.install_state == PresetUpdaterVendorRowState::InstallState::Queued
            || vendor.install_state == PresetUpdaterVendorRowState::InstallState::Running)
        {
            installing = true;
        }
        if (vendor.install_state == PresetUpdaterVendorRowState::InstallState::Done) {
            continue;
        }
        switch (vendor.state) {
        case VendorReconfigurationState::Update:
            ++source.counts.updates;
            break;
        case VendorReconfigurationState::NewVendor:
            ++source.counts.new_vendors;
            break;
        case VendorReconfigurationState::ForcedUpdate:
        case VendorReconfigurationState::ForcedDowngrade:
            ++source.counts.required;
            break;
        case VendorReconfigurationState::NotInIndex:
            ++source.counts.unknown_versions;
            break;
        }
    }

    if (!source.selected) {
        source.update_state = PresetUpdaterSourceRowState::UpdateState::NotChecked;
    } else if (installing) {
        source.update_state = PresetUpdaterSourceRowState::UpdateState::Installing;
    } else if (source.counts.pending() > 0) {
        source.update_state = PresetUpdaterSourceRowState::UpdateState::HasUpdates;
    } else {
        source.update_state = PresetUpdaterSourceRowState::UpdateState::UpToDate;
    }
}

void PresetUpdaterModel::on_preset_updater_error(
    Biz::PresetUpdater::JobId job_id,
    const std::string& body,
    Biz::PresetUpdater::PresetUpdaterReason reason
)
{
    SPDLOG_ERROR("Preset updater job {} failed: {}", std::to_string(job_id), body);

    const auto subject = m_job_subjects.find(job_id);
    m_activity_reporter.report_error(
        reason, subject == m_job_subjects.end() ? std::string{} : subject->second
    );

    if (!dialog_open()) {
        return;
    }

    const auto install_job = m_install_jobs.find(job_id);
    if (install_job != m_install_jobs.end()) {
        for (const VendorKey& key : install_job->second.keys) {
            mutate_vendor(
                key,
                [&body](PresetUpdaterVendorRowState& vendor)
                {
                    vendor.install_state = PresetUpdaterVendorRowState::InstallState::Failed;
                    vendor.error_text    = body;
                }
            );
        }
    }

    m_operation_failed = true;
    set_status(Status::Warned);
}

void PresetUpdaterModel::on_preset_updater_forced_reconfigurations_list(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    log_reconfigurations(reconfigurations);
    log_warnings(warnings);
    set_problem_warnings(warnings);

    // Opening the dialog starts the listing and check these vendors need.
    update_forced_vendors(reconfigurations);
    if (!m_forced_vendors.empty()) {
        notify_changed();
        show_dialog();
        return;
    }

    // Deliberately not tracked as an activity: nobody asked for this one.
    m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(
        online_allowed(), Biz::PresetUpdater::VerboseStyle::NoProgress
    );
}

void PresetUpdaterModel::on_preset_updater_reconfigurations_list(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::PresetUpdaterReconfigurationList& reconfigurations,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings,
    Biz::PresetUpdater::VerboseStyle verbose
)
{
    log_reconfigurations(reconfigurations);
    log_warnings(warnings);

    std::map<std::string, std::vector<PresetUpdaterVendorRowState>> by_repo;
    for (const Biz::PresetUpdater::VendorReconfiguration& reconfiguration :
         all_reconfigurations(reconfigurations))
    {
        PresetUpdaterVendorRowState vendor;
        vendor.repo_id             = reconfiguration.vendor_repo_id;
        vendor.vendor_id           = reconfiguration.vendor_id;
        vendor.comment             = reconfiguration.comment;
        vendor.state               = reconfiguration.state;
        vendor.current_version     = reconfiguration.current_version;
        vendor.recommended_version = reconfiguration.recommended_version;
        by_repo[reconfiguration.vendor_repo_id].push_back(std::move(vendor));
    }

    std::map<std::string, std::set<std::string>> skipped_by_repo;
    std::set<std::string> failed_repos;
    for (const Biz::PresetUpdater::PresetUpdaterWarning& warning : warnings) {
        if (!warning.repo.empty() && !warning.vendor.empty()) {
            skipped_by_repo[warning.repo].insert(warning.vendor);
            continue;
        }
        // A warning naming a source but no vendor is the check failing before it read any of them.
        if (!warning.repo.empty()) {
            failed_repos.insert(warning.repo);
        }
    }
    set_problem_warnings(warnings);

    for (const auto& [repo_id, vendor_ids] : skipped_by_repo) {
        for (const std::string& vendor_id : vendor_ids) {
            PresetUpdaterVendorRowState vendor;
            vendor.repo_id   = repo_id;
            vendor.vendor_id = vendor_id;
            vendor.skipped   = true;
            by_repo[repo_id].push_back(std::move(vendor));
        }
    }

    static const std::vector<PresetUpdaterVendorRowState> k_nothing_offered;

    mutate_all_sources(
        [this, &by_repo, &skipped_by_repo, &failed_repos](PresetUpdaterSourceRowState& source)
        {
            if (!source.selected) {
                source.vendors->reset(std::vector<PresetUpdaterVendorRowState>{});
                source.skipped_vendors.clear();
                source.check_failed = false;
                source.counts       = PresetUpdaterSourceCounts{};
                source.update_state = PresetUpdaterSourceRowState::UpdateState::NotChecked;
                return;
            }

            const auto vendors = by_repo.find(source.id);
            merge_vendor_rows(source, vendors != by_repo.end() ? vendors->second : k_nothing_offered);

            const auto skipped = skipped_by_repo.find(source.id);
            if (skipped != skipped_by_repo.end()) {
                source.skipped_vendors.assign(skipped->second.begin(), skipped->second.end());
            } else {
                source.skipped_vendors.clear();
            }

            source.check_failed = failed_repos.contains(source.id);

            refresh_source_counts(source);
        }
    );

    update_forced_vendors(reconfigurations);
    refresh_status();

    if (has_forced_reconfigurations()) {
        show_dialog();
        return;
    }

    // Only what is on offer: presets of a version the source does not list cannot be installed.
    const size_t update_count = reconfigurations.regular_updates().size()
        + reconfigurations.new_vendors().size();

    m_activity_reporter.report_check_finished(update_count);
}

void PresetUpdaterModel::on_preset_updater_reconfigurations_performed(
    Biz::PresetUpdater::JobId job_id,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    log_warnings(warnings);

    // Above the lookup below: presets are on disk whoever asked for the install.
    if (m_presets_installed_callback) {
        m_presets_installed_callback();
    }

    const auto install_job = m_install_jobs.find(job_id);
    if (install_job == m_install_jobs.end()) {
        return;
    }

    // A warning naming a vendor is that one's install having failed; the others went through.
    const auto failure_of = [&warnings](const VendorKey& key)
    {
        return std::find_if(
            warnings.begin(),
            warnings.end(),
            [&key](const Biz::PresetUpdater::PresetUpdaterWarning& warning) {
                return warning.repo == key.repo_id && warning.vendor == key.vendor_id;
            }
        );
    };

    std::vector<PresetUpdaterActivityReporter::InstalledVendor> installed;
    for (const PresetUpdaterActivityReporter::InstalledVendor& vendor :
         install_job->second.installed)
    {
        const auto failed = std::find_if(
            warnings.begin(),
            warnings.end(),
            [&vendor](const Biz::PresetUpdater::PresetUpdaterWarning& warning)
            { return warning.vendor == vendor.vendor_id; }
        );
        if (failed == warnings.end()) {
            installed.push_back(vendor);
        }
    }

    // Not guarded on the dialog: a row left running would hold its source's tick locked for good.
    m_activity_reporter.report_install_finished(installed);
    set_problem_warnings(warnings);

    for (const VendorKey& key : install_job->second.keys) {
        const auto failure = failure_of(key);
        if (failure != warnings.end()) {
            const std::string error_text = failure->text;
            mutate_vendor(
                key,
                [&error_text](PresetUpdaterVendorRowState& vendor)
                {
                    vendor.install_state = PresetUpdaterVendorRowState::InstallState::Failed;
                    vendor.error_text    = error_text;
                }
            );
            continue;
        }

        // A forced vendor whose install failed is still forced.
        std::erase_if(
            m_forced_vendors,
            [&key](const VendorKey& forced)
            { return forced.repo_id == key.repo_id && forced.vendor_id == key.vendor_id; }
        );
        mutate_vendor(
            key,
            [](PresetUpdaterVendorRowState& vendor)
            {
                vendor.install_state = PresetUpdaterVendorRowState::InstallState::Done;
                vendor.error_text.clear();
            }
        );
    }

    refresh_status();
}

void PresetUpdaterModel::on_preset_updater_status(
    Biz::PresetUpdater::JobId job_id,
    const std::string& target,
    int attempt,
    unsigned delay,
    Biz::PresetUpdater::VerboseStyle verbose
)
{
    SPDLOG_INFO(
        "Preset updater status: target:{} attempt:{} delay:{}", target, attempt, delay
    );

    // Above the dialog guard: the notification keeps naming the target after the dialog closes.
    m_activity_reporter.report_progress(job_id, target, attempt);

    if (!dialog_open()) {
        return;
    }

    // The only signal telling a queued install job that its turn has come.
    const auto install_job = m_install_jobs.find(job_id);
    if (install_job == m_install_jobs.end()) {
        return;
    }

    for (const VendorKey& key : install_job->second.keys) {
        mutate_vendor(
            key,
            [](PresetUpdaterVendorRowState& vendor)
            {
                if (vendor.install_state == PresetUpdaterVendorRowState::InstallState::Queued) {
                    vendor.install_state = PresetUpdaterVendorRowState::InstallState::Running;
                }
            }
        );
    }

    notify_changed();
}

void PresetUpdaterModel::on_preset_updater_repository_info_vector(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    log_warnings(warnings);

    // Above the dialog guard: adding or removing a source can outlive the dialog.
    set_problem_warnings(warnings);

    if (!dialog_open()) {
        return;
    }

    rebuild_sources(descriptor);
    start_check();
}

void PresetUpdaterModel::on_preset_updater_repository_selection_performed(
    Biz::PresetUpdater::JobId job_id,
    const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& descriptor,
    const std::vector<Biz::PresetUpdater::PresetUpdaterWarning>& warnings
)
{
    log_warnings(warnings);

    if (m_commit_dirty) {
        // The follow-up commit is issued from on_preset_updater_job_finished; check after that.
        return;
    }

    m_working_selection = descriptor;
    set_problem_warnings(warnings);

    // Above the dialog guard: a selection committed on close is still owed its check.
    if (!dialog_open()) {
        m_activity_reporter.begin_activity(
            m_preset_updater_interactor.build_update_sync_and_reconfiguration_check(
                online_allowed(),
                Biz::PresetUpdater::VerboseStyle::NoProgress,
                false,
                Biz::PresetUpdater::SourceListSync::UseStored
            ),
            Activity::Checking
        );
        return;
    }

    if (needs_check()) {
        start_check();
    } else {
        refresh_status();
    }
}

void PresetUpdaterModel::on_preset_updater_job_finished(
    Biz::PresetUpdater::JobId job_id, Biz::PresetUpdater::JobState state
)
{
    m_job_subjects.erase(job_id);

    // Unguarded by the dialog: selections, activities and rows all outlive it.
    if (job_id == m_selection_job) {
        m_selection_job = Biz::PresetUpdater::k_invalid_job_id;
        if (m_commit_dirty) {
            commit_selection();
        }
    }

    m_activity_reporter.end_activity(job_id);

    const auto install_job = m_install_jobs.find(job_id);
    if (install_job != m_install_jobs.end()) {
        if (state != Biz::PresetUpdater::JobState::Succeeded) {
            for (const VendorKey& key : install_job->second.keys) {
                mutate_vendor(
                    key,
                    [state](PresetUpdaterVendorRowState& vendor)
                    {
                        if (vendor.install_state
                            != PresetUpdaterVendorRowState::InstallState::Done)
                        {
                            vendor.install_state = state == Biz::PresetUpdater::JobState::Canceled ?
                                PresetUpdaterVendorRowState::InstallState::Idle :
                                PresetUpdaterVendorRowState::InstallState::Failed;
                        }
                    }
                );
            }
        }
        m_install_jobs.erase(install_job);
    }

    if (job_id == m_check_job) {
        m_check_job = Biz::PresetUpdater::k_invalid_job_id;
        if (state == Biz::PresetUpdater::JobState::Failed) {
            // A cancelled check is left alone: a replacement is already on its way.
            mutate_all_sources(
                [](PresetUpdaterSourceRowState& source)
                {
                    if (source.update_state == PresetUpdaterSourceRowState::UpdateState::Checking
                        || source.update_state == PresetUpdaterSourceRowState::UpdateState::Waiting)
                    {
                        source.update_state = PresetUpdaterSourceRowState::UpdateState::NotChecked;
                    }
                }
            );
        }
    }

    const bool forced_check_over = job_id == m_forced_check_job;
    if (forced_check_over) {
        m_forced_check_job = Biz::PresetUpdater::k_invalid_job_id;
    }

    notify_changed();

    if (forced_check_over && m_forced_check_finished_callback) {
        m_forced_check_finished_callback(has_forced_reconfigurations());
    }
}

} // namespace Slic3r::App
