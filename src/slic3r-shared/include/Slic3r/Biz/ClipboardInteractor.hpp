#pragma once

#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

#include <mutex>
#include <queue>
#include <set>
#include <vector>

namespace Slic3r::Biz {
class ArrangeInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Scene {
class SceneInteractor;
enum class SelectionMode;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::Domain {
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class ClipboardInteractor
{
public:
    ClipboardInteractor() = delete;
    ClipboardInteractor(
        Scene::SceneInteractor& scene_interactor,
        ArrangeInteractor& arrange_interactor,
        const Domain::Workbench& workbench
    );
    ~ClipboardInteractor() = default;

    /**
     * @brief Returns true if the current selection is non-empty and can be copied.
     */
    bool can_copy() const;

    /**
     * @brief Returns true if the clipboard has valid content that can be pasted.
     */
    bool can_paste() const;

    /**
     * @brief Stores the current selection for later pasting.
     * @param project_id ID of the project containing the selected objects.
     */
    void copy(Domain::SelectionId project_id);

    /**
     * @brief Pastes stored selection into the currently selected project.
     * @param project_id ID of the target project to paste into.
     */
    void paste(Domain::SelectionId project_id);

private:
    struct Clipboard
    {
        Domain::SelectionId source_project_id{Domain::INVALID_ID};
        Scene::SelectionMode mode{};
        Domain::ElementRefs selected_elements;

        bool is_empty() const;
        void clear();
    };

    struct PendingArrange
    {
        Domain::SelectionId project_id{Domain::INVALID_ID};
        std::set<size_t> object_ids;
        Domain::BedRef target_bed;
    };

    void paste_objects(Domain::SelectionId project_id);
    void paste_volumes(Domain::SelectionId project_id);

    void process_partial_arrange_queue();

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

    Scene::SceneInteractor& m_scene_interactor;
    ArrangeInteractor& m_arrange_interactor;
    const Domain::Workbench& m_workbench;

    Clipboard m_clipboard;

    std::queue<PendingArrange> m_partial_arrange_queue;
    std::mutex m_arrange_mutex;
};

} // namespace Slic3r::Biz
