#pragma once

#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/ISlicingInputChangedListener.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/PrintHost/PrintHostInteractor.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"

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
    public WithListeners<
        ISelectedProjectChangedListener,
        IProjectsChangedListener,
        ISelectedConfigContainerChangedListener
    >
{
public:
    explicit ProjectInteractor(Domain::Workbench& workbench, Platform::IMainThreadDispatcher& dispatcher)
        : m_workbench(workbench), m_preset_interactor(workbench), m_scene_interactor(workbench), m_slicing_interactor(dispatcher), m_print_host_interactor(dispatcher)
    {
        add_listener<ISelectedConfigContainerChangedListener>(&m_preset_interactor);
        add_listener<ISelectedConfigContainerChangedListener>(&m_scene_interactor);
        add_listener<ISelectedProjectChangedListener>(&m_scene_interactor);
        m_scene_interactor.add_listener<ISelectedBedInstanceChangedListener>(this);
        add_listener<ISelectedProjectChangedListener>(&m_slicing_interactor);
        m_scene_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_preset_interactor.add_listener<ISlicingInputChangedListener>(this);
        m_slicing_interactor.add_listener<Slicing::IFDMResultListener>(&m_fdm_result_cache);
    }

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
     */
    void do_export(const Slicing::SlicingId id, const boost::filesystem::path& dest_path);

     /**
     * @brief Creates PrintHostConfig and PrintHostData and passes it to PrintHostInteractor to start upload.
     * PrintHostData copies gcode data from m_fdm_result_cache.
     * PrintHostConfig origin is yet to be decided.
     */
    void do_upload(const Slicing::SlicingId id);

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
    PrintHost::PrintHostInteractor m_print_host_interactor;
};

} // namespace Slic3r::Biz
