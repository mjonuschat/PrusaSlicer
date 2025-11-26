#pragma once

#include "Slic3r/Biz/ArrangeInteractor.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"
#include "Slic3r/Biz/SLAResultCache.hpp"
#include "Slic3r/Biz/SLAObjectCache.hpp"
#include "Slic3r/Biz/StatusCache.hpp"
#include "Slic3r/Biz/ISlicingInputChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/InvokeLaterBag.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostInteractor.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountInteractor.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"
#include "Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.hpp"
#include "Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactory.hpp"
#include "Slic3r/Biz/AppInstance/IAppInstanceMessageContentListener.hpp"
#include "Slic3r/Biz/ObservableProjectList.hpp"
#include "Slic3r/Biz/Format/3mf.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterInteractor.hpp"
#include "Slic3r/Biz/RemovableDrive/RemovableDriveService.hpp"
#include "Slic3r/Biz/FileDownloader/FileDownloaderInteractor.hpp"

#include "Slic3r/Biz/FileLoadingLogic.hpp"

namespace Slic3r::Domain {
class Project;
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class ISelectedProjectChangedListener;
class ISelectedConfigContainerChangedListener;
class IProjectsChangedListener;
class IMessageDialogProvider;

/**
 * @brief Top level interactor managing list of projects and their bed selection.
 *
 * Use this interactor to:
 * - manipulate project add/close/select,
 * - get access to PresetInteractor (to modify config container of bed),
 * - get access to SceneInteractor (to manipulate 3D volumes)
 * .
 */

class ProjectInteractor final :
    public ISelectedBedInstancesChangedListener,
    public ISlicingInputChangedListener,
    public UserAccount::IUserAccountListener,
    public AppInstance::IAppInstanceMessageContentListener,
    public Scene::ISceneChangedListener,
    public FileDownloader::IFileDownloaderListener,
    public WithListeners<ISelectedProjectChangedListener, IProjectsChangedListener, ISelectedConfigContainerChangedListener>
{
public:
    ProjectInteractor(Domain::Workbench& workbench, Platform::IMainThreadDispatcher& dispatcher, Biz::Slicing::IThumbnailImageGenerator& thumbnail_image_generator) :
        m_workbench(workbench),
        m_scene_interactor(workbench),
        m_preset_interactor(workbench, m_scene_interactor),
        m_arrange_interactor(m_scene_interactor, workbench),
        m_slicing_interactor(dispatcher, thumbnail_image_generator),
        m_print_host_interactor(dispatcher),
        m_user_account_interactor(dispatcher),
        m_app_instance_message_handler(AppInstance::create_app_instance_message_handler(dispatcher)),
        m_preset_updater_interactor(dispatcher),
        m_removable_drive_service(dispatcher),
        m_project_list(*this),
        m_file_downloader_interactor(dispatcher)
    {
        add_listener<ISelectedConfigContainerChangedListener>(&m_preset_interactor);
        add_listener<ISelectedConfigContainerChangedListener>(&m_scene_interactor);
        add_listener<ISelectedProjectChangedListener>(&m_scene_interactor);
        add_listener<ISelectedProjectChangedListener>(&m_preset_interactor);
        m_scene_interactor.add_listener<ISelectedBedInstancesChangedListener>(this);
        m_scene_interactor.add_listener<Scene::ISceneChangedListener>(this);
        add_listener<ISelectedProjectChangedListener>(&m_slicing_interactor);
        m_scene_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_preset_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_preset_interactor.add_listener<Preset::IPresetChangedListener>(&m_scene_interactor);
        m_slicing_interactor.set_listener<Slicing::IFDMResultListener>(&m_fdm_result_cache);
        m_slicing_interactor.set_listener<Slicing::ISLAResultListener>(&m_sla_result_cache);
        m_slicing_interactor.set_listener<Slicing::ISLAObjectListener>(&m_sla_object_cache);
        m_slicing_interactor.add_listener<Slicing::IStatusListener>(&m_status_cache);
        m_user_account_interactor.add_listener<UserAccount::IUserAccountListener>(this);
        m_app_instance_message_handler->add_listener<AppInstance::IAppInstanceMessageContentListener>(this);
        m_file_downloader_interactor.add_listener<FileDownloader::IFileDownloaderListener>(this);
        m_scene_interactor.add_listener<Scene::ISceneSelectionChangedListener>(
            &m_preset_interactor.object_settings_interactor()
        );
        add_listener<ISelectedConfigContainerChangedListener>(
            &m_preset_interactor.object_settings_interactor()
        );
    }

    const Domain::Workbench& workbench() const
    {
        return m_workbench;
    }

    void initialize_bed(Domain::ConfigContainer& config_container, Domain::BedContainer& bed_container);

    /**
     * @name Project manipulation
     * @{
     */
    /**
     * @brief Create new project
     * @return Returns ID of newly created project
     */
    Domain::SelectionId new_project();
    Domain::SelectionId new_project_with_modification(
        const std::function<void(Domain::Project&)>& modifier
    );

    /**
     * @name Project manipulation
     * @{
     */
    /**
     * @brief Load project from the file
     */
    void load_project(const boost::filesystem::path& file_path);

    /**
     * @name Project manipulation
     * @{
     */
    /**
     * @brief Save project into the file
     */
    void save_project(const boost::filesystem::path& file_path, const Store3mfParam& params);

    /**
     * Select already opened project. If the project is already selected, do nothing.
     * @param project_id An index of project to be selected.
     */
    void select_project(Domain::SelectionId project_id);

    /**
     * Add project to Workbench and select it.
     * @param p Project to move to workbench
     * @return A project_id / index of added project.
     */
    Domain::SelectionId add_project(Domain::Project&& p);

    /**
     * Remove given project from workbench and update selection if needed.
     * @param project_id
     */
    void remove_project(Domain::SelectionId project_id);

    /**
     * Renames project and push changes to ObservableProjectsList
     * @param project_id
     * @param new_name
     * @note project_id needs to be valid
     */
    void rename_project(Domain::SelectionId project_id, const std::string& new_name);

    /** @} */

    /**
     * @name Config container operations
     * @{
     */

    Domain::SelectionId add_config_container();
    Domain::SelectionId duplicate_config_container(Domain::SelectionId config_container_id);
    void remove_config_container(Domain::SelectionId config_container_id);
    void select_config_container(Domain::SelectionId config_container_id);

    /** @} */

    /**
     * @name Selection ID getters
     * @{
     */

    /**
     * @brief Get selected project ID
     */
    Domain::SelectionId selected_project_id() const
    {
        return m_selection.project_id;
    }

    /**
     * @brief Get selected config container ID
     */
    Domain::SelectionId selected_config_container_id() const
    {
        return m_selection.config_container_id();
    }

    /** @} */
    /**
     * @name Quick selection getters
     * @{
     */
    const Domain::Project& selected_project() const
    {
        DEBUG_ASSERT(m_selection.project_id != Domain::INVALID_ID);
        return m_workbench.project(m_selection.project_id);
    }

    Domain::Project& selected_project()
    {
        DEBUG_ASSERT(m_selection.project_id != Domain::INVALID_ID);
        return m_workbench.project(m_selection.project_id);
    }

    const Domain::ConfigContainer& selected_config_container() const
    {
        DEBUG_ASSERT(m_selection.config_container_id() != Domain::INVALID_ID);
        return *DEBUG_ASSERT_VAL(selected_project().find_config_container(m_selection.config_container_id()));
    }

    Domain::ConfigContainer& selected_config_container()
    {
        DEBUG_ASSERT(m_selection.config_container_id() != Domain::INVALID_ID);
        return *DEBUG_ASSERT_VAL(selected_project().find_config_container(m_selection.config_container_id()));
    }

    /** @} */

    /**
     * @brief Get immutable preset interactor
     * @return Immutable preset interactor instance
     */
    const Preset::PresetInteractor& preset_interactor() const
    {
        return m_preset_interactor;
    }

    /**
     * @brief Get mutable preset interactor
     * @return Mutable preset interactor instance
     */
    Preset::PresetInteractor& preset_interactor()
    {
        return m_preset_interactor;
    }

    /**
     * @brief Get immutable scene interactor
     * @return Immutable scene interactor instance
     */
    const Scene::SceneInteractor& scene_interactor() const
    {
        return m_scene_interactor;
    }

    /**
     * @brief Get mutable scene interactor
     * @return Mutable scene interactor instance
     */
    Scene::SceneInteractor& scene_interactor()
    {
        return m_scene_interactor;
    }

    Slicing::SlicingInteractor& slicing_interactor()
    {
        return m_slicing_interactor;
    }

    FDMResultCache& fdm_result_cache()
    {
        return m_fdm_result_cache;
    }

    SLAResultCache& sla_result_cache()
    {
        return m_sla_result_cache;
    }

    SLAObjectCache& sla_object_cache()
    {
        return m_sla_object_cache;
    }

    StatusCache& status_cache()
    {
        return m_status_cache;
    }

    Biz::ArrangeInteractor& arrange_interactor()
    {
        return m_arrange_interactor;
    }

    Domain::SlicingId selected_bed_slicing_id() const;

    void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs &instances) override;

    /**
     * @name ISelectedBedInstancesChangedListener interface implementation
     * @{
     */
    void on_selected_bed_instances_changed(Domain::SelectionId project_id, const Scene::BedSelection& selection) override;
    /** @} */

    /**
     * @brief Creates PrintHostConfig and PrintHostData and passes it to PrintHostInteractor to start export.
     * PrintHostData copies gcode data from m_fdm_result_cache.
     * PrintHostConfig origin is yet to be decided.
     */
    void do_export(const Domain::SlicingId id, const boost::filesystem::path& dest_path);

    /**
     * @brief Creates PrintHostConfig and PrintHostData and passes it to PrintHostInteractor to start upload.
     * PrintHostData copies gcode data from m_fdm_result_cache.
     * PrintHostConfig origin is yet to be decided.
     */
    void do_upload(const Domain::SlicingId id, const std::string& filename);

    /**
     * @brief Same as do_upload, but does parse connect_msg first.
     * Uploads to PrintHostType::PrusaConnect.
     */
    void do_upload_connect(const Domain::SlicingId id, const std::string& connect_msg);

    /**
     * @brief Called after Mainframe is created to set window handle for AppInstanceMessageHandler.
     */
    void init_app_instance_message_handler(void* window_handle)
    {
        m_app_instance_message_handler->init(window_handle);
    }

    /**
     * @brief Passes message to AppInstanceMessageHandler.
     */
    void handle_app_instance_message(const std::string& message)
    {
        m_app_instance_message_handler->handle_message(message);
    }

    /**
     * @brief Called on every successful login to user account and token renewal.
     * Notifies all other running apps to read token store.
     */
    void on_user_account_id_success(bool is_refresh, const std::string& username) override
    {
        m_app_instance_message_handler
            ->multicast_message("STORE_READ", {}, Biz::Platform::PlatformServices::instance().app_hash());
    }

    /**
     * @brief Called on performed log out.
     * Notifies all other running apps to read token store.
     * This listener should be triggered from UserAccount only when logout was NOT caused by accepted STORE_READ.
     */
    void on_user_account_logged_out() override
    {
        m_app_instance_message_handler
            ->multicast_message("STORE_READ", {}, Biz::Platform::PlatformServices::instance().app_hash());
    }

    void on_user_account_will_refresh() override
    { /*unused*/
    }

    void on_user_account_action_retry(const Network::IHttp::Retry& retry, std::function<void(void)> cancel_callback) override
    { /*unused*/
    }

    void load_models_to_project(std::vector<boost::filesystem::path> paths);

    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_open_models(std::vector<boost::filesystem::path> paths) override
    {
        load_models_to_project(paths);
    }

    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_download_models(std::vector<std::string> message) override 
    {
        if (raise_app_fn) {
            raise_app_fn();
        }
        m_file_downloader_interactor.download_files_prusaslicer_url(message);
    }

    void download_model_from_printables_tab(FileDownloader::FileDownloaderJobInput data)
    {
        if (data.new_project) {
            new_project();
        }
        m_file_downloader_interactor.init_download_job(std::move(data));
    }

    /**
     * @brief Callback from FileDownloader.
     */

    void on_model_downloaded(const boost::filesystem::path& path) override
    {
        load_models_to_project({path});
    }

    void open_downloaded_file(const boost::filesystem::path& path, bool in_new_project)
    {
        if (in_new_project) {
            new_project();
        }        
        load_models_to_project({path});
    }

    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_read_token_store_message() override
    {
        m_user_account_interactor.on_read_token_store_message();
    }

    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_login_data(const std::string& message) override
    {
        m_user_account_interactor.on_log_in_code_response(message);
    }

    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_app_go_front() override {}

    /**
     * @brief Getter for user account.
     */
    UserAccount::UserAccountInteractor& user_account_interactor()
    {
        return m_user_account_interactor;
    }

    /**
     * @brief Getter for exporting / uploading logic.
     */
    PrintHost::PrintHostInteractor& print_host_interactor()
    {
        return m_print_host_interactor;
    }

    std::string get_project_name(Domain::SelectionId project_id) const;

    
    /**
     * @brief Getter for default path when exporting 3mf file.
     */
    const boost::filesystem::path& export_project_path(Domain::SelectionId project_id) const;
    void set_export_project_path(Domain::SelectionId project_id, const boost::filesystem::path& path);

    /**
     * @brief Getter for default path when exporting gcode.
     */
    boost::filesystem::path export_result_path(Domain::SelectionId project_id, bool only_removable) const;
    void set_export_result_path(Domain::SelectionId project_id, const boost::filesystem::path& path);

    ObservableProjectList& observable_project_list();

    /**
     * @brief Getter for Preset Updater.
     */
    PresetUpdater::PresetUpdaterInteractor& preset_updater_interactor()
    {
        return m_preset_updater_interactor;
    }

    /**
     * @brief Getter for RemovableDriveService.
     */
    RemovableDrive::RemovableDriveService& removable_drive_service()
    {
        return m_removable_drive_service;
    }

    /**
     * @brief Passing information from system about changed volumes to Removable Drive service.
     */
    void handle_volumes_changed_event()
    {
        m_removable_drive_service.handle_volumes_changed_event();
    }

    void set_dialog_provider(IMessageDialogProvider* dialog_provider);

    /*
     * @brief Sets callback to mainframe to bring application forward.
     */
    void set_raise_app_fn(std::function<void(void)> fn)
    {
        raise_app_fn = fn;
    }

private:
    void on_slicing_input_changed(const Domain::BedRef& bed_instance) override;
    void on_slicing_input_removed(const Domain::BedRef& bed_instance) override;

    Domain::SelectionId add_project(Domain::Project&& p, InvokeLaterBag& bag);
    void do_select_project(Domain::SelectionId project_id, InvokeLaterBag& bag);
    void do_select_config_container(Domain::SelectionId container_id);

private:
    struct Selection
    {
        Domain::SelectionId project_id{Domain::INVALID_ID};
        std::map<Domain::SelectionId, Domain::SelectionId> project_config_container;

        Domain::SelectionId config_container_id() const;
        void set_config_container_id(Domain::SelectionId container_id);
    };

    Domain::Workbench& m_workbench;
    Selection m_selection;

    Scene::SceneInteractor m_scene_interactor;
    Preset::PresetInteractor m_preset_interactor;
    ArrangeInteractor m_arrange_interactor;
    Slicing::SlicingInteractor m_slicing_interactor;
    FDMResultCache m_fdm_result_cache;
    SLAResultCache m_sla_result_cache;
    SLAObjectCache m_sla_object_cache;
    StatusCache m_status_cache;
    PrintHost::PrintHostInteractor m_print_host_interactor;
    UserAccount::UserAccountInteractor m_user_account_interactor;
    std::unique_ptr<AppInstance::AbstractAppInstanceMessageHandler> m_app_instance_message_handler;
    PresetUpdater::PresetUpdaterInteractor m_preset_updater_interactor;
    RemovableDrive::RemovableDriveService m_removable_drive_service;
    FileDownloader::FileDownloaderInteractor m_file_downloader_interactor;

    ObservableProjectList m_project_list;
    IMessageDialogProvider* m_dialog_provider{ nullptr };

    /*
     * @brief Callback to mainframe to bring application forward.
     */
    std::function<void(void)> raise_app_fn;
};

} // namespace Slic3r::Biz
