#include "Slic3r/Biz/ClipboardInteractor.hpp"

#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/ArrangeInteractor.hpp"
#include "Slic3r/Biz/IUndoProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelInstance.hpp"
#include "Slic3r/Domain/ModelObject.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/Workbench.hpp"

using namespace Slic3r;
using namespace Slic3r::Biz;

using Slic3r::Biz::Scene::ObjectSelection;
using Slic3r::Biz::Scene::SelectionMode;
using Slic3r::Domain::BedInstance;
using Slic3r::Domain::BedRef;
using Slic3r::Domain::ElementRef;
using Slic3r::Domain::ElementRefs;
using Slic3r::Domain::ModelInstance;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::ModelObjectPtrs;
using Slic3r::Domain::ModelVolume;
using Slic3r::Domain::Project;
using Slic3r::Domain::SelectionId;
using Slic3r::Domain::Transformation;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Workbench;

namespace Slic3r::Biz {

const Arrange::Settings ARRANGE_SETTINGS{
    .strategy        = Arrange::Strategy::Gravity,
    .scaled_offset   = Algorithms::Scaling::scaled(3.0),
    .allow_rotations = false
};

ClipboardInteractor::ClipboardInteractor(
    Scene::SceneInteractor& scene_interactor,
    ArrangeInteractor& arrange_interactor,
    const Workbench& workbench
) :
    m_scene_interactor{scene_interactor},
    m_arrange_interactor{arrange_interactor},
    m_workbench{workbench}
{}

bool ClipboardInteractor::can_copy() const
{
    const ObjectSelection& object_selection = m_scene_interactor.object_selection();
    return !object_selection.empty() && !object_selection.contains_wipe_tower();
}

bool ClipboardInteractor::can_paste() const
{
    if (m_clipboard.is_empty()) {
        return false;
    } else if (!m_workbench.projects().contains(m_clipboard.source_project_id)) {
        return false;
    } else if (m_clipboard.mode == SelectionMode::Volume
               && !m_scene_interactor.object_selection().only_single_object())
    {
        // We allow pasting volume only when single object is sellected.
        return false;
    }

    const Project& source_project = m_workbench.project(m_clipboard.source_project_id);
    const bool any_source_exists  = std::ranges::any_of(
        m_clipboard.selected_elements,
        [&source_project](const ElementRef& element)
        { return source_project.find_object_by_id(element.object_id) != nullptr; }
    );

    return any_source_exists;
}

void ClipboardInteractor::copy(const SelectionId project_id)
{
    const ObjectSelection& selection = m_scene_interactor.object_selection();
    if (selection.empty()) {
        return;
    }

    m_clipboard.clear();
    m_clipboard.source_project_id = project_id;
    m_clipboard.mode              = selection.mode;
    m_clipboard.selected_elements = selection.elements;
}

void ClipboardInteractor::paste(const SelectionId project_id)
{
    if (m_clipboard.is_empty()) {
        return;
    }

    if (m_clipboard.mode == SelectionMode::Volume) {
        this->paste_volumes(project_id);
    } else {
        this->paste_objects(project_id);
    }
}

void ClipboardInteractor::Clipboard::clear()
{
    this->source_project_id = Domain::INVALID_ID;
    this->mode              = SelectionMode::Instance;
    this->selected_elements.clear();
}

bool ClipboardInteractor::Clipboard::is_empty() const
{
    return selected_elements.empty();
}

void ClipboardInteractor::paste_objects(const SelectionId project_id)
{
    ModelObjectPtrs new_objects = m_scene_interactor.clone_objects_from_project(
        m_clipboard.source_project_id,
        m_clipboard.selected_elements
    );

    std::set<size_t> new_object_ids;
    for (const ModelObject* new_object : new_objects) {
        new_object_ids.insert(new_object->id().id);
    }

    if (new_object_ids.empty()) {
        return;
    }

    const BedRef target_bed = m_scene_interactor.bed_selection().last_selected_bed();

    bool is_queue_processing_running = false;
    {
        std::lock_guard lock(m_arrange_mutex);
        is_queue_processing_running = !m_partial_arrange_queue.empty();
        m_partial_arrange_queue.push({project_id, std::move(new_object_ids), target_bed});
    }

    if (!is_queue_processing_running) {
        this->process_partial_arrange_queue();
    }
}

void ClipboardInteractor::process_partial_arrange_queue()
{
    PendingArrange pending_arrange;
    {
        std::lock_guard lock(m_arrange_mutex);

        if (m_partial_arrange_queue.empty()) {
            m_scene_interactor.undo_provider().take_snapshot(UndoSnapshotType::PasteObjects);
            return;
        }

        pending_arrange = m_partial_arrange_queue.front();
    }

    m_arrange_interactor.partial_arrange(
        pending_arrange.project_id,
        pending_arrange.object_ids,
        pending_arrange.target_bed,
        ARRANGE_SETTINGS,
        [this](const ElementRefs& not_arranged)
        {
            if (!not_arranged.empty()) {
                const PendingArrange& current         = m_partial_arrange_queue.front();
                const SelectionId project_id          = current.project_id;
                const BedRef& target_bed              = current.target_bed;
                const Project& project                = m_workbench.project(project_id);
                const SelectionId config_container_id = current.target_bed.config_container_id;

                const Domain::ConfigContainer* config_container =
                    project.find_config_container(config_container_id);
                ASSERT(config_container != nullptr);

                const BedRef last_bed_in_container{
                    config_container_id,
                    config_container->bed_instances().back()->id().id
                };

                BedRef next_bed = last_bed_in_container;
                if (target_bed == last_bed_in_container) {
                    // Create a new bed and arrange objects there.
                    const BedInstance& new_bed =
                        m_scene_interactor.add_bed_instance(config_container_id);
                    next_bed = BedRef{config_container_id, new_bed.id().id};
                    m_scene_interactor.bed_selection().toggle(next_bed);
                }

                std::set<size_t> not_arranged_ids;
                for (const ElementRef& ref : not_arranged) {
                    not_arranged_ids.insert(ref.object_id);
                }

                this->move_instances_to_bed(project_id, not_arranged, next_bed);

                {
                    std::lock_guard lock(m_arrange_mutex);
                    m_partial_arrange_queue.pop();
                    m_partial_arrange_queue.push(
                        {project_id, std::move(not_arranged_ids), next_bed}
                    );
                }
            } else {
                std::lock_guard lock(m_arrange_mutex);

                m_partial_arrange_queue.pop();
            }

            this->process_partial_arrange_queue();
        }
    );
}

void ClipboardInteractor::move_instances_to_bed(
    const SelectionId project_id,
    const ElementRefs& instances,
    const BedRef& bed_ref
)
{
    const BedInstance* bed_instance =
        m_workbench.project(project_id).find_bed_instance_by_id(bed_ref.instance_id);
    if (bed_instance == nullptr) {
        return;
    }

    const Vec2d bed_offset =
        Algorithms::Point::to_2d(Transformation{bed_instance->transformation}.get_offset());
    const Vec2d bed_center = bed_offset + bed_instance->bed.get().center();

    Arrange::InstanceTransforms trafos;
    for (const ElementRef& ref : instances) {
        trafos.push_back(
            {.instance_ref = ref, .absolute_offset = bed_center, .rotation_delta = 0.0}
        );
    }

    m_scene_interactor.transform_instances(trafos);
}

void ClipboardInteractor::paste_volumes(SelectionId project_id)
{
    ASSERT(m_clipboard.mode == SelectionMode::Volume);
    ASSERT(!m_clipboard.selected_elements.empty());

    const ObjectSelection& selection = m_scene_interactor.object_selection();
    if (selection.empty() || !selection.only_single_object()) {
        return;
    }

    const Project& source_project    = m_workbench.project(m_clipboard.source_project_id);
    const size_t source_object_id    = m_clipboard.selected_elements.front().object_id;
    const ModelObject* source_object = source_project.find_object_by_id(source_object_id);
    if (source_object == nullptr) {
        return;
    }

    std::set<size_t> selected_volume_ids;
    for (const ElementRef& selected_element : m_clipboard.selected_elements) {
        if (selected_element.has_volume()) {
            selected_volume_ids.insert(selected_element.volume_id);
        }
    }

    if (selected_volume_ids.empty()) {
        return;
    }

    const size_t selection_instance_id = selection.elements.front().instance_id;
    for (const ModelVolume* source_volume : source_object->volumes) {
        if (!selected_volume_ids.contains(source_volume->id().id)) {
            continue;
        }

        m_scene_interactor.add_volume(
            project_id,
            selection_instance_id,
            [source_volume](ModelObject& target_object) -> ModelVolume*
            {
                ModelVolume* new_volume = target_object.add_volume(*source_volume);
                new_volume->set_new_unique_id();
                return new_volume;
            }
        );
    }

    m_scene_interactor.undo_provider().take_snapshot(UndoSnapshotType::PasteVolumes);
}

} // namespace Slic3r::Biz
