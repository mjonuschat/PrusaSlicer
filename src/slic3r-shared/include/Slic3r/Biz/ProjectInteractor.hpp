#pragma once

#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/StatusCache.hpp"
#include "Slic3r/Biz/ISlicingInputChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostInteractor.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/LastExportPathStorage.hpp"
#include "Slic3r/Biz/UserAccount/UserAccountInteractor.hpp"
#include "Slic3r/Biz/UserAccount/IUserAccountListener.hpp"
#include "Slic3r/Biz/AppInstance/AbstractAppInstanceMessageHandler.hpp"
#include "Slic3r/Biz/AppInstance/AppInstanceMessageHandlerFactory.hpp"
#include "Slic3r/Biz/AppInstance/IAppInstanceMessageContentListener.hpp"

namespace Slic3r::Domain {
class Project;
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class ISelectedProjectChangedListener;
class ISelectedConfigContainerChangedListener;
class IProjectsChangedListener;

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
    public ISelectedBedInstanceChangedListener,
    public ISlicingInputChangedListener,
    public UserAccount::IUserAccountListener,
    public AppInstance::IAppInstanceMessageContentListener,
    public WithListeners<
        ISelectedProjectChangedListener,
        IProjectsChangedListener,
        ISelectedConfigContainerChangedListener
    >
{
public:
    explicit ProjectInteractor(Domain::Workbench& workbench, Platform::IMainThreadDispatcher& dispatcher)
        : m_workbench(workbench), m_preset_interactor(workbench), m_scene_interactor(workbench), m_slicing_interactor(dispatcher), m_print_host_interactor(dispatcher)
        , m_user_account_interactor(dispatcher), m_app_instance_message_handler(AppInstance::create_app_instance_message_handler(dispatcher))
    {
        add_listener<ISelectedConfigContainerChangedListener>(&m_preset_interactor);
        add_listener<ISelectedConfigContainerChangedListener>(&m_scene_interactor);
        add_listener<ISelectedProjectChangedListener>(&m_scene_interactor);
        m_scene_interactor.add_listener<ISelectedBedInstanceChangedListener>(this);
        add_listener<ISelectedProjectChangedListener>(&m_slicing_interactor);
        m_scene_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_preset_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_slicing_interactor.set_listener<Slicing::IFDMResultListener>(&m_fdm_result_cache);
        m_slicing_interactor.add_listener<Slicing::IStatusListener>(&m_status_cache);
        m_user_account_interactor.add_listener<UserAccount::IUserAccountListener>(this);
        m_app_instance_message_handler->add_listener<AppInstance::IAppInstanceMessageContentListener>(this);
    }

    const Domain::Workbench& workbench() const { return m_workbench; }

    /**
     * @name Project manipulation
     * @{
     */
    /**
     * @brief Create new project
     * @return Returns ID of newly created project
     */
    Domain::SelectionId new_project();      

    /**
     * @name Project manipulation
     * @{
     */
    /**
     * @brief Load project from the file
     * @return Returns ID of newly created project
     */
    Domain::SelectionId load_project(const std::string& file_path);

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

    /** @} */

    /**
     * @name Config container operations
     * @{
     */

    void add_config_container();
    void remove_config_container(Domain::SelectionId config_container_id);

    /** @} */

    /**
     * @name Selection ID getters
     * @{
     */

    /**
     * @brief Get selected project ID
     */
    Domain::SelectionId selected_project_id() const { return m_selection.project_id; }

    /**
     * @brief Get selected config container ID
     */
    Domain::SelectionId selected_config_container_id() const { return m_selection.config_container_id; }

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
        DEBUG_ASSERT(m_selection.config_container_id != Domain::INVALID_ID);
        return *DEBUG_ASSERT_VAL(
            selected_project().find_config_container(m_selection.config_container_id)
        );
    }

    Domain::ConfigContainer& selected_config_container()
    {
        DEBUG_ASSERT(m_selection.config_container_id != Domain::INVALID_ID);
        return *DEBUG_ASSERT_VAL(
            selected_project().find_config_container(m_selection.config_container_id)
        );
    }
    /** @} */

    /**
     * @brief Get immutable preset interactor
     * @return Immutable preset interactor instance
     */
    const Preset::PresetInteractor& preset_interactor() const { return m_preset_interactor; }

    /**
     * @brief Get mutable preset interactor
     * @return Mutable preset interactor instance
     */
    Preset::PresetInteractor& preset_interactor() { return m_preset_interactor; }

    /**
     * @brief Get immutable scene interactor
     * @return Immutable scene interactor instance
     */
    const Scene::SceneInteractor& scene_interactor() const { return m_scene_interactor; }

    /**
     * @brief Get mutable scene interactor
     * @return Mutable scene interactor instance
     */
    Scene::SceneInteractor& scene_interactor() { return m_scene_interactor; }

    Slicing::SlicingInteractor& slicing_interactor() { return m_slicing_interactor; }

    FDMResultCache& fdm_result_cache() { return m_fdm_result_cache; }
    StatusCache& status_cache() { return m_status_cache; }
    Biz::Slicing::SlicingId selected_bed_slicing_id() const;

    /**
     * @name ISelectedBedInstanceChangedListener interface implementation
     * @{
     */
    void on_selected_bed_instance_changed(Domain::SelectionId project_id, Domain::SelectionId container_id, Domain::SelectionId bed_instance_id) override;
    /** @} */

    /**
     * @brief Creates PrintHostConfig and PrintHostData and passes it to PrintHostInteractor to start export.
     * PrintHostData copies gcode data from m_fdm_result_cache.
     * PrintHostConfig origin is yet to be decided.
     * to_removable parameter is placeholder until more robust logic takes place.
     */
    void do_export(const Slicing::SlicingId id, const boost::filesystem::path& dest_path, bool to_removable);

     /**
     * @brief Creates PrintHostConfig and PrintHostData and passes it to PrintHostInteractor to start upload.
     * PrintHostData copies gcode data from m_fdm_result_cache.
     * PrintHostConfig origin is yet to be decided.
     */
    void do_upload(const Slicing::SlicingId id, const std::string& filename);

    /**
     * @brief Same as do_upload, but does parse connect_msg first.
     * Uploads to PrintHostType::PrusaConnect.
     */
    void do_upload_connect(const Slicing::SlicingId id, const std::string& connect_msg);

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
    void on_user_account_id_success(bool is_refresh) override
    {
        m_app_instance_message_handler->multicast_message("STORE_READ", {}, Biz::Platform::PlatformServices::instance().app_hash());
    }

    /**
     * @brief Called on performed log out.
     * Notifies all other running apps to read token store.
     * This function should be called only when logout was NOT caused by accepted STORE_READ.
     */
    void  on_user_account_logged_out() override
    {
        m_app_instance_message_handler->multicast_message("STORE_READ", {}, Biz::Platform::PlatformServices::instance().app_hash());
    }
   
    void on_user_account_will_refresh() override { /*unused*/}

    /**
    * @brief Callback from AppInstanceMessageHandler.
    */
    void on_open_models(std::vector<boost::filesystem::path> message) override {}
    
    /**
     * @brief Callback from AppInstanceMessageHandler.
     */
    void on_download_models(std::vector<std::string> message) override {}
    
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
    UserAccount::UserAccountInteractor& user_account_interactor() { return m_user_account_interactor; }

    std::string get_project_name(Domain::SelectionId project_id) const;

    boost::filesystem::path last_export_path(bool only_removable) const { return m_last_export_path_storage.get_last_export_path(only_removable); }

    void set_last_export_path(const boost::filesystem::path& path, bool is_removable) { m_last_export_path_storage.set_last_export_path(path, is_removable); }

private:
    void on_slicing_input_changed(const Domain::BedRef& bed_instance) override;
    void on_slicing_input_removed(const Domain::BedRef& bed_instance) override;


    void do_select_project(Domain::SelectionId project_id);
    void do_select_config_container(Domain::SelectionId container_id);

    void initialize_new_project_before_inserting(Domain::Project& p);
    void initialize_inserted_project(size_t project_id);

private:
    struct Selection
    {
        Domain::SelectionId project_id{Domain::INVALID_ID};
        Domain::SelectionId config_container_id{Domain::INVALID_ID};
    };

    Domain::Workbench& m_workbench;
    Selection m_selection;

    Preset::PresetInteractor m_preset_interactor;
    Scene::SceneInteractor m_scene_interactor;
    Slicing::SlicingInteractor m_slicing_interactor;
    FDMResultCache m_fdm_result_cache;
    StatusCache m_status_cache;
    PrintHost::PrintHostInteractor m_print_host_interactor;
    LastExportPathStorage m_last_export_path_storage;
    UserAccount::UserAccountInteractor m_user_account_interactor;
    std::unique_ptr<AppInstance::AbstractAppInstanceMessageHandler> m_app_instance_message_handler;
};

} // namespace Slic3r::Biz
