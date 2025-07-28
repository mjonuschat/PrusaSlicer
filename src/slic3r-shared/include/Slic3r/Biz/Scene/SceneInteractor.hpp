//
#pragma once
#include <unordered_map>

#include "Slic3r/Biz/ISlicingInputChangedListener.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Scene/SceneInteractorProjectContext.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Biz/Scene/BedPlacement.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Domain { class Bed; }

namespace Slic3r::Biz {
class ISelectedBedInstancesChangedListener;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Scene {

class ISceneSelectionChangedListener
{
public:
    virtual ~ISceneSelectionChangedListener() = default;
    virtual void on_scene_selection_changed(Domain::SelectionId project_id, const ObjectSelection& selection) = 0;
    virtual void on_scene_selection_transformed(Domain::SelectionId project_id, const ObjectSelection& selection) {};
};

enum class TransformState
{
    InProgress, /* Transform is in progress (i.e. user is dragging the mouse) */
    Completed,  /* Transform was completed (i.e. user released the mouse button) */
    Canceled    /* Transform was canceled (i.e. user pressed ESC key) */
};

class ISceneChangedListener
{
public:
    virtual ~ISceneChangedListener() = default;

    /**
     * @brief Called whenever new instances are added.
     * @remark The callee is responsible to create all volume representations for given instances.
     * @param project_id Project the new instances belong to
     * @param instances List of instances to add
     */
    virtual void on_instance_added(Domain::SelectionId project_id, const Domain::ElementRefs& instances) = 0;
    /**
     * @brief Called whenever instances are removed.
     * @param project_id Project the instances belong to
     * @param instances List of instances to remove
     */
    virtual void on_instance_removed(Domain::SelectionId project_id, const Domain::ElementRefs& instances) = 0;

    /**
     * @brief Called whenever instances are transformed.
     * @param project_id Project the instances belong to
     * @param elements List of instances to transform
     * @param state Indicates whether the transform state is final (interactive incremental transform)
     */
    virtual void on_instance_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, TransformState state) = 0;

    /**
     * @brief Called whenever volumes are added
     * @remark The callee is responsible to create volume representations of @p volumes for all existing instances.
     * @param project_id Project the volumes belong to
     * @param volumes Set of volumes to add.
     */
    virtual void on_volume_added(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) = 0;

    /**
     * @brief Called whenever volumes are removed
     * @remark The callee is responsible to remove volume representations of @p volumes from all existing instances.
     * @param project_id Project the volumes belong to
     * @param volumes Set of volumes to add.
     */
    virtual void on_volume_removed(Domain::SelectionId project_id, const Domain::ElementRefs& volumes) = 0;

    /**
     * @brief Called whenever volumes are tranformed.
     * @param project_id Project the instances belong to
     * @param elements List of instances to transform
     * @param state Indicates whether the transform state is final (interactive incremental transform)
     */
    virtual void on_volume_transformed(Domain::SelectionId project_id, const Domain::ElementRefs& elements, TransformState state) = 0;

    virtual void on_wipe_tower_added(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_removed(Domain::SelectionId project_id, size_t idx) = 0;
    virtual void on_wipe_tower_transformed(Domain::SelectionId project_id, size_t idx, TransformState state) = 0;
};

class ISceneBedInstanceChangedListener
{
public:
    virtual ~ISceneBedInstanceChangedListener() = default;

    virtual void on_bed_instance_added(Domain::SelectionId project_id, const Domain::BedRefs& instances) {};
    virtual void on_bed_instance_removed(Domain::SelectionId project_id, const Domain::BedRefs& instances) {};
    virtual void on_bed_instance_transformed(Domain::SelectionId project_id, const Domain::BedRefs& instances, TransformState state) {};
};

struct TransformMemento;

class SceneInteractor final :
    public ISelectedProjectChangedListener,
    public ISelectedConfigContainerChangedListener,
    public WithListeners<
        ISceneSelectionChangedListener,
        ISceneChangedListener,
        ISceneBedInstanceChangedListener,
        ISelectedBedInstancesChangedListener,
        ISlicingInputChangedListener>
{
public:
    using Transform = Domain::SquareMatrix4d;

    explicit SceneInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    void on_selected_project_changed(size_t index) override;
    void on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id) override;

    void new_object_from_mesh(Domain::TriangleMesh&& mesh, const std::string& name = std::string());
    void add_new_objects(const std::vector<Domain::ModelObject*>& objects);
    void add_volume_from_mesh(
        Domain::TriangleMesh&& mesh,
        Domain::ModelVolumeType volume_type,
        const std::string& name = std::string(),
        const Transform& xform  = Domain::SquareMatrix4d::Identity()
    );
    void add_instance(const Domain::Vec2d& offset);
    void notify_listener_on_objects(const std::vector<Domain::ModelObject*>& objects);
    void notify_listener_on_objects();

    using RefMesh = std::pair<Domain::ElementRef, Domain::TriangleMesh>;
    using RefMeshes = std::vector<RefMesh>;
    void change_volume_meshes(RefMeshes&& meshes);
    void edit_name(const Domain::ElementRef& id, const std::string& new_name);
    void set_printable(const Domain::ElementRef& id, bool is_printable);
    void extract_selected_instances();
    /**
     * Delete elements (volumes or instances) from the current scene selection.
     *
     * @return An optional string containing the name of the last solid part that was attempted to be deleted.
     *         Deleting the last solid part is not permitted, and the user will be informed about it later.
     */
    std::optional<std::string> delete_selected_elements();

    void prepare_loaded_project(Domain::Project& project);

    Domain::BedInstance& add_bed_instance(size_t config_container_id);
    void remove_bed_instance(const Domain::BedRef& instance);
    void transform_bed_instance(const Domain::BedRef& instance, const Transform& xform);

    const Domain::Project::ConfigContainerList& selected_project_config_containers() const;
    const Domain::ModelInstanceList& unplaced_model_instances(const Domain::SelectionId project_id) const;
    const Domain::ModelInstanceList& selected_project_unplaced_model_instances() const;
    const Domain::BedContainer::BedList& selected_project_beds() const;


    Domain::SelectionId selected_config_container_id() const
    { return m_selected_config_container_id; }

    /**
     * @name Scene selection
     * @{
     */
    const ObjectSelection& object_selection() const;
    void set_object_selection(const ObjectSelection& selection);
    void modify_selection(const std::function<void(ObjectSelection&)>& modifier);
    /** @} */

    const BedSelection& bed_selection() const;
    BedSelection& bed_selection();
    const BedSelection* bed_selection(const Domain::SelectionId project_id) const;
    BedSelection* bed_selection(const Domain::SelectionId project_id);

    /**
     * @name Transforming selection
     * @{
     */
    /**
     * @breif Update transform of all selected volumes or instances in a interactive way.
     *
     * This method will start or continue (depending on the @p memento object state) transforming
     * selected volumes or instances (depending on selection mode).
     *
     * @param relative_transform Relative transformation (w.r.t. object transformation at the start
     * of transform change) to be applied to all objects in selection.
     * @param memento Maintains state of the transformation (i.e. original transformation at time of
     * transform change start).
     *
     * @note It is required to call finalize_transform_selection() to finish the operation.
     */
    void transform_selection(const Transform& relative_transform, TransformMemento& memento);

    /**
     * @brief Update selection transform in one shot (not interactive way).
     * @param relative_transform Relative transformation (w.r.t. object transformation at the start
     * of transform change) to be applied to all objects in selection.
     *
     * @note This effectively same as calling transform_selection(const Transform&, TransformMemento&)
     * and then finalize_transform_selection()
     */
    void transform_selection(const Transform& relative_transform);

    struct Trafo {
        Domain::ElementRef instance_ref;
        Domain::Vec2d absolute_offset;
        double rotation_delta;
    };

    using Trafos = std::vector<Trafo>;
    void transform_instances(const Trafos& transformations);

    /**
     * @brief Finalize or cancel selection transform.
     * @param memento State of the transform in progress.
     * @param canceled Indicates if the transform in progress was canceled or finished.
     */
    void finalize_transform_selection(TransformMemento& memento, bool canceled);
    /** @} */

private:
    void update_selection_instance_bed_placement();
    void invoke_slicing_input_changed(const Domain::BedRef& bed_instance);

private:
    using ProjectContexts = std::unordered_map<Domain::SelectionId, SceneInteractorProjectContext>;

    Domain::Workbench& m_workbench;

    ProjectContexts m_projects;
    Domain::SelectionId m_selected_project_id {Domain::INVALID_ID};
    Domain::SelectionId m_selected_config_container_id {Domain::INVALID_ID};
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
