//
#pragma once
#include "libslic3r/TriangleMesh.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ListenerList.hpp"
#include "Slic3r/Domain/ElementRef.hpp"

namespace Slic3r::Biz::Scene {

class ISceneSelectionChangedListener
{
public:
    virtual ~ISceneSelectionChangedListener() = default;
    virtual void on_scene_selection_changed(Domain::SelectionId project_id, const Selection& selection) = 0;
};

class ISceneChangedListener
{
public:
    virtual ~ISceneChangedListener() = default;

    virtual void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) = 0;
    virtual void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) = 0;
    virtual void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) = 0;

    virtual void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) = 0;
    virtual void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) = 0;
    virtual void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements) = 0;
    virtual void on_volume_mesh_changed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) = 0;

    virtual void on_bed_added(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_bed_removed(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_bed_transformed(Domain::SelectionId project_id, size_t idx) = 0;

    virtual void on_wipe_tower_added(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx) = 0;
};


class SceneInteractor final : public ISelectedProjectChangedListener
{
public:
    explicit SceneInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    void on_selected_project_changed(size_t index) override;
    void new_object_from_mesh(TriangleMesh&& mesh);

    /**
     * @name Scene selection
     * @{
     */
    const Selection& selection() const;
    void set_selection(const Selection& selection);
    void modify_selection(const std::function<void(Selection&)>& modifier);
    /** @} */


    /**
     * @name Selection Changed Listener
     * @{
     */
    void add_scene_selection_changed_listener(ISceneSelectionChangedListener* l)
    {
        m_selection_changed_listeners.add(l);
    }

    void remove_scene_selection_changed_listener(ISceneSelectionChangedListener* l)
    {
        m_selection_changed_listeners.remove(l);
    }
    /** @} */

    /**
     * @name Scene Changed Listener
     * @{
     */
    void add_scene_changed_listener(ISceneChangedListener* l)
    {
        m_changed_listeners.add(l);
    }

    void remove_scene_changed_listener(ISceneChangedListener* l)
    {
        m_changed_listeners.remove(l);
    }
    /** @} */
private:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, SceneInteractorProjectContext>;

    Domain::Workbench& m_workbench;

    ProjectContexts m_projects;
    Domain::SelectionId m_selected_project_id {Domain::INVALID_ID};
    Biz::ListenerList<ISceneSelectionChangedListener> m_selection_changed_listeners;
    Biz::ListenerList<ISceneChangedListener> m_changed_listeners;
};

} // namespace Slic3r::Biz::Interactor
