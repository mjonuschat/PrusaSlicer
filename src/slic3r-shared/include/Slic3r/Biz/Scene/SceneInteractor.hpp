//
#pragma once
#include <unordered_map>

#include "libslic3r/TriangleMesh.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Biz/Scene/BedPlacement.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/BedRef.hpp"

namespace Slic3r::Domain { class Bed; }

namespace Slic3r::Biz {
class ISelectedBedInstanceChangedListener;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Scene {

class ISceneSelectionChangedListener
{
public:
    virtual ~ISceneSelectionChangedListener() = default;
    virtual void on_scene_selection_changed(Domain::SelectionId project_id, const Selection& selection) = 0;
    virtual void on_scene_selection_transformed(Domain::SelectionId project_id, const Selection& selection) = 0;
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

    virtual void on_bed_instance_added(Domain::SelectionId project_id, const Domain::BedRefs& instances) = 0;
    virtual void on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances) = 0;
    virtual void on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances) = 0;

    virtual void on_wipe_tower_added(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx) = 0;
};

struct TransformMemento;

class SceneInteractor final : public ISelectedProjectChangedListener,
                              public ISelectedConfigContainerChangedListener
{
public:
    using Transform = Matrix4d;

    explicit SceneInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    void on_selected_project_changed(size_t index) override;
    void on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id) override;

    void new_object_from_mesh(TriangleMesh&& mesh);
    void add_volume_from_mesh(TriangleMesh&& mesh, ModelVolumeType volume_type, const Transform& xform = Matrix4d::Identity());
    void add_instance(const Transform& xform);

    Domain::BedInstance& add_bed_instance(size_t config_container_id);
    void remove_bed_instance(const Domain::BedRef& instance);
    void transform_bed_instance(const Domain::BedRef& instance, const Transform& xform);

    void select_bed_instance(const Domain::BedRef& instance);
    void select_first_bed_instance();
    const Domain::BedRef& selected_bed_instance() const { return m_selected_bed_instance; }

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

    /**
     * @name Selected Bed Instance Changed Listener
     * @{
     */
    void add_bed_instance_selection_changed_listener(ISelectedBedInstanceChangedListener* l)
    {
        m_bed_instance_selection_changed_listeners.add(l);
    }

    void remove_bed_instance_selection_changed_listener(ISelectedBedInstanceChangedListener* l)
    {
        m_bed_instance_selection_changed_listeners.remove(l);
    }
    /** @} */

    /**
     * @name Transforming selection
     * @{
     */
    /**
     * @breif Update transform of all selected volumes or instances.
     *
     * This method will start or continue (depending on the @p memento object state) transforming
     * selected volumes or instances (depending on selection mode).
     *
     * @param relative_transform Relative transformation (w.r.t. object transformation at the start
     * of transform change) to be applied to all objects in selection.
     * @param memento Maintains state of the transformation (i.e. original transformation at time of
     * transform change start).
     */
    void transform_selection(const Transform& relative_transform, TransformMemento& memento);

    /**
     * @brief Finalize or cancel selection transform.
     * @param memento State of the transform in progress.
     * @param canceled Indicates if the transform in progress was canceled or finished.
     */
    void finalize_transform_selection(TransformMemento& memento, bool canceled);
    /** @} */
private:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, SceneInteractorProjectContext>;

    Domain::Workbench& m_workbench;

    ProjectContexts m_projects;
    Domain::SelectionId m_selected_project_id {Domain::INVALID_ID};
    Domain::SelectionId m_selected_config_container_id {Domain::INVALID_ID};
    Biz::ListenerList<ISceneSelectionChangedListener> m_selection_changed_listeners;
    Biz::ListenerList<ISelectedBedInstanceChangedListener> m_bed_instance_selection_changed_listeners;
    Biz::ListenerList<ISceneChangedListener> m_changed_listeners;
    Domain::BedRef m_selected_bed_instance{ Domain::INVALID_ID, Domain::INVALID_ID };
    BedPlacement m_bed_placement;
};

struct TransformMemento
{
    struct Element
    {
        Domain::ElementRef element;
        SceneInteractor::Transform original_xform;
    };
    using Elements = std::unordered_map<Domain::ElementRef, Element>;

    Elements elements;

    void reset() { elements.clear(); }
};


} // namespace Slic3r::Biz::Interactor
