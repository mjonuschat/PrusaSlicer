#pragma once

#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Arrange/Settings.hpp"
#include "Slic3r/Biz/IArrangeEventsListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Exception.hpp"

#include <functional>
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
     * @brief Arranges a subset of objects on a single bed while keeping all others fixed.
     * @param project_id ID of the target project.
     * @param arrangeable_object_ids Object IDs to arrange (everything else is fixed).
     * @param target_bed Bed to arrange on.
     * @param settings Arrangement settings.
     * @param on_completed Optional callback invoked on the main thread after the arrangement finishes,
     *        receiving elements that could not be arranged on the target bed.
     */
    void partial_arrange(
        Domain::SelectionId project_id,
        const std::set<size_t>& arrangeable_object_ids,
        const Domain::BedRef& target_bed,
        const Biz::Arrange::Settings& settings,
        PartialArrangeCallback on_completed = {}
    );

private:
    Scene::SceneInteractor& m_scene_interactor;
    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
    const Domain::Workbench& m_workbench;

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
};
} // namespace Slic3r::Biz
