#pragma once

#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Arrange/Settings.hpp"
#include "Slic3r/Biz/IArrangeEventsListener.hpp"
#include "Slic3r/Biz/IUndoProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Exception.hpp"

#include <functional>
#include <mutex>
#include <queue>
#include <set>

namespace Slic3r::Biz {

struct ArrangeBed
{
    const Domain::BedInstance& bed_instance;
    const double offset{};
};

using ArrangeBeds = std::vector<ArrangeBed>;

struct ArrangeFatalError : public Exception
{
    using Exception::Exception;
};

struct Pack
{
    Arrange::InstanceTransforms trafos;
    Domain::BoundingBox2d bounding_box;
};

using Packs = std::vector<Pack>;

class ArrangeInteractor : public WithListeners<IArrangeEventsListener>
{
public:
    ArrangeInteractor(Scene::SceneInteractor& scene_interactor, const Domain::Workbench& workbench);

    void arrange(
        const Domain::SelectionId project_id,
        const Biz::Arrange::Settings& settings,
        std::function<void()> on_finished
    );

    using PartialArrangeCallback = std::function<void(const Domain::ElementRefs& not_arranged)>;

    /**
     * @brief Arranges a subset of instances on a single bed while keeping all others fixed.
     * @param project_id ID of the target project.
     * @param arrangeable_instance_ids Instance IDs to arrange (everything else is fixed).
     * @param target_bed Bed to arrange on.
     * @param settings Arrangement settings.
     * @param on_completed Optional callback invoked on the main thread after the arrangement finishes,
     *        receiving elements that could not be arranged on the target bed.
     */
    void partial_arrange(
        Domain::SelectionId project_id,
        const std::set<size_t>& arrangeable_instance_ids,
        const Domain::BedRef& target_bed,
        const Biz::Arrange::Settings& settings,
        PartialArrangeCallback on_completed = {}
    );

    /**
     * @brief Auto-arranges freshly added instances on the target bed, keeping every other
     *        instance fixed, and takes a single undo snapshot once the operation finished.
     *
     * @param project_id ID of the target project.
     * @param added_instances Instance-level element refs of the newly added items to arrange.
     * @param target_bed Bed to arrange the new instances on.
     * @param snapshot_type Undo the snapshot type.
     */
    void arrange_added_instances(
        Domain::SelectionId project_id,
        const Domain::ElementRefs& added_instances,
        const Domain::BedRef& target_bed,
        UndoSnapshotType snapshot_type
    );

private:
    Scene::SceneInteractor& m_scene_interactor;
    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    const Domain::Workbench& m_workbench;

    struct PendingArrange
    {
        Domain::SelectionId project_id{Domain::INVALID_ID};
        std::set<size_t> instance_ids;
        Domain::BedRef target_bed;
        UndoSnapshotType snapshot_type{};
    };

    std::queue<PendingArrange> m_added_arrange_queue;
    std::mutex m_added_arrange_mutex;

    void process_added_arrange_queue();

    Domain::ConstModelInstanceList get_model_instances(
        const Domain::SelectionId project_id,
        const Scene::BedSelection& selection,
        const bool include_unplaced
    ) const;

    enum class OverflowMode
    {
        AddBeds,
        MoveNextToFirstBed
    };

    double apply_arrange_result(
        const Scene::BedInstances& bed_instances,
        const double scaled_offset,
        const std::vector<Pack>& packs,
        const double initial_offset,
        Domain::ElementRefs* not_arranged
    );

    double apply_arrange_result(
        const Domain::SelectionId project_id,
        const Scene::BedSelection& selection,
        const OverflowMode& overflow_mode,
        const double scaled_offset,
        const std::vector<Pack>& packs,
        const double initial_offset,
        Domain::ElementRefs* not_arranged = nullptr
    );

    double apply_arrange_result(
        const Domain::SelectionId project_id,
        const Domain::BedRef& bed_ref,
        const double scaled_offset,
        const std::vector<Pack>& packs,
        const double initial_offset,
        Domain::ElementRefs* not_arranged = nullptr
    );

    /**
     * @brief Moves given instances to the origin of the specified bed.
     * @param project_id Project containing the instances.
     * @param instances Elements to move.
     * @param bed_ref Target bed.
     */
    void move_instances_to_bed(
        Domain::SelectionId project_id,
        const Domain::ElementRefs& instances,
        const Domain::BedRef& bed_ref
    );
};
} // namespace Slic3r::Biz
