#include "Slic3r/App/Undo/Store.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Undo {

Store::Store(
    Biz::ProjectInteractor& project_interactor
) :
    m_project_interactor{project_interactor},
    m_scene_interactor{project_interactor.scene_interactor()},
    m_workbench{project_interactor.workbench()}
{
    m_project_interactor.add_listener<Biz::IProjectsChangedListener>(this);
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);
}

Store::~Store()
{
    m_project_interactor.remove_listener<Biz::IProjectsChangedListener>(this);
    m_project_interactor.remove_listener<Biz::ISelectedProjectChangedListener>(this);
}

static std::optional<std::size_t> get_undo_stack_index(
    Biz::UndoSnapshotSelection::Variant snapshot_variant,
    const Stack& undo_redo_stack
)
{
    const std::vector<Snapshot>& snapshots{undo_redo_stack.get_snapshots()};
    const std::optional<std::size_t> selected_index_opt{undo_redo_stack.get_selected_index()};
    if (!selected_index_opt) {
        return std::nullopt;
    }

    const int selected_index{static_cast<int>(*selected_index_opt)};
    const int index_to_load{std::visit(
        Domain::overloaded{
            [&](std::size_t id)
            {
                const auto it{std::ranges::find_if(
                    snapshots,
                    [&](const Snapshot& snapshot) { return snapshot.id == id; }
                )};
                return static_cast<int>(std::distance(snapshots.begin(), it));
            },
            [&](Biz::UndoSnapshotSelection::Next) { return selected_index + 1; },
            [&](Biz::UndoSnapshotSelection::Prev) { return selected_index - 1; },
        },
        snapshot_variant
    )};

    if (index_to_load < 0 || index_to_load >= snapshots.size()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(index_to_load);
}

static std::string to_string(Biz::UndoSnapshotType type)
{
    using Type = Biz::UndoSnapshotType;
    using Biz::_u8L;
    switch (type) {
    case Type::None:
        PANIC("None snapshot type!");
    case Type::InitializeProject:
        return _u8L("Initialize project");
    case Type::QuickDrag:
        return _u8L("Quick drag");
    case Type::QuickDragAndAddBed:
        return _u8L("Quick drag and add a bed");
    case Type::Translate:
        return _u8L("Translate");
    case Type::SetTranslation:
        return _u8L("Set translation");
    case Type::Rotate:
        return _u8L("Rotate");
    case Type::SetRotation:
        return _u8L("Set rotation");
    case Type::RevertRotation:
        return _u8L("Revert rotation");
    case Type::Scale:
        return _u8L("Scale");
    case Type::SetScale:
        return _u8L("Set scale");
    case Type::RevertScale:
        return _u8L("Revert scale");
    case Type::PlaceOnFace:
        return _u8L("Place on face");
    case Type::Arrange:
        return _u8L("Arrange");
    case Type::Cut:
        return _u8L("Cut");
    case Type::AddObject:
        return _u8L("Add an object");
    case Type::AddInstance:
        return _u8L("Add an instance");
    case Type::DelInstance:
        return _u8L("Delete an instance");
    case Type::SetNumberOfInstances:
        return _u8L("Set number of instances");
    case Type::AddVolume:
        return _u8L("Load a volume");
    case Type::AddVolumeCube:
        return _u8L("Add a volume cube");
    case Type::AddVolumeCylinder:
        return _u8L("Add a volume cylinder");
    case Type::AddVolumeSphere:
        return _u8L("Add a volume sphere");
    case Type::AddVolumeText:
        return _u8L("Add a text volume");
    case Type::AddVolumeSvg:
        return _u8L("Add a volume from SVG");
    case Type::AddVolumeGallery:
        return _u8L("Add a volume from gallery");
    case Type::AddNegativeVolume:
        return _u8L("Load a negative volume");
    case Type::AddNegativeVolumeCube:
        return _u8L("Add a negative cube");
    case Type::AddNegativeVolumeCylinder:
        return _u8L("Add a negative cylinder");
    case Type::AddNegativeVolumeSphere:
        return _u8L("Add a negative sphere");
    case Type::AddNegativeVolumeText:
        return _u8L("Add a negative text volume");
    case Type::AddNegativeVolumeSvg:
        return _u8L("Add a negative volume from SVG");
    case Type::AddNegativeVolumeGallery:
        return _u8L("Add a negative volume from gallery");
    case Type::AddModifier:
        return _u8L("Load a modifier volume");
    case Type::AddModifierCube:
        return _u8L("Add a modifier cube");
    case Type::AddModifierCylinder:
        return _u8L("Add a modifier cylinder");
    case Type::AddModifierSphere:
        return _u8L("Add a modifier sphere");
    case Type::AddModifierText:
        return _u8L("Add a modifier text volume");
    case Type::AddModifierSvg:
        return _u8L("Add a modifier volume from SVG");
    case Type::AddModifierGallery:
        return _u8L("Add a modifier volume from gallery");
    case Type::AddSupportBlocker:
        return _u8L("Load a support blocker");
    case Type::AddSupportBlockerCube:
        return _u8L("Add a support blocker cube");
    case Type::AddSupportBlockerCylinder:
        return _u8L("Add a support blocker cylinder");
    case Type::AddSupportBlockerSphere:
        return _u8L("Add a support blocker sphere");
    case Type::AddSupportBlockerGallery:
        return _u8L("Add a support blocker from gallery");
    case Type::AddSupportModifier:
        return _u8L("Add a support modifier");
    case Type::AddSupportModifierCube:
        return _u8L("Add a support enforcer cube");
    case Type::AddSupportModifierCylinder:
        return _u8L("Add a support enforcer cylinder");
    case Type::AddSupportModifierSphere:
        return _u8L("Add a support enforcer sphere");
    case Type::AddSupportModifierGallery:
        return _u8L("Add a support enforcer from gallery");
    case Type::AddCube:
        return _u8L("Add a cube");
    case Type::AddCylinder:
        return _u8L("Add a cylinder");
    case Type::AddSphere:
        return _u8L("Add a sphere");
    case Type::DeleteSelection:
        return _u8L("Delete selection");
    case Type::PaintOnSupportsStroke:
        return _u8L("Support painting stroke");
    case Type::PaintOnSeamsStroke:
        return _u8L("Seam painting stroke");
    case Type::PaintOnFuzzySkinStroke:
        return _u8L("Fuzzy skin painting stroke");
    case Type::MMPaintingStroke:
        return _u8L("Multi material painting stroke");
    case Type::ActivateGizmo:
        return _u8L("Open tool dialog");
    case Type::DeactivateGizmo:
        return _u8L("Close tool dialog");
    case Type::SelectBed:
        return _u8L("Select a bed");
    case Type::AddConfigContainer:
        return _u8L("Add config container");
    case Type::DeleteConfigContainer:
        return _u8L("Delete config container");
    case Type::AddBed:
        return _u8L("Add a bed");
    case Type::DeleteBed:
        return _u8L("Delete a bed");
    case Type::SelectPrinterPreset:
        return _u8L("Select a printer");
    case Type::SetPartSettingsValue:
        return _u8L("Set part settings value");
    case Type::SetAsSeparateObject:
        return _u8L("Selected instances to objects");
    case Type::SplitToObjects:
        return _u8L("Split to objects");
    case Type::SplitToVolumes:
        return _u8L("Split to volumes");
    case Type::MergeToOneObject:
        return _u8L("Merge to object");
    case Type::InvalidateCutInfo:
        return _u8L("Invalidate Cut Info");
    case Type::SetAsPrintable:
        return _u8L("Change printable state");
    case Type::ChangeVolumeType:
        return _u8L("Change volume type");
    }
    PANIC("Unknown option");
    return {};
}

void Store::select_snapshot(Biz::UndoSnapshotSelection::Variant snapshot_variant)
{
    Domain::SelectionId project_id{m_project_interactor.selected_project_id()};
    const auto it{m_stacks.find(project_id)};
    if (it == m_stacks.end()) {
        return;
    }

    Stack& stack{it->second};
    const std::optional<std::size_t> index_to_load{get_undo_stack_index(snapshot_variant, stack)};

    if (!index_to_load) {
        return;
    }

    const std::vector<Snapshot>& snapshots{stack.get_snapshots()};

    Domain::Project& project{m_project_interactor.project(project_id)};

    LoadedSnapshot loaded_snapshot{stack.load_and_select_snapshot(
        project_id,
        snapshots.at(*index_to_load),
        project.bed_container(),
        m_project_interactor.preset_interactor()
    )};

    m_snapshotting_enabled = false;
    const ScopeGuard guard{[&]() { m_snapshotting_enabled = true; }};

    m_project_interactor.reload_config_containers_after_undo(
        project_id,
        std::move(loaded_snapshot.config_containers)
    );

    m_scene_interactor.bed_selection().set_state(
        loaded_snapshot.bed_selection_state.selected_beds,
        loaded_snapshot.bed_selection_state.selected_config_container,
        loaded_snapshot.bed_selection_state.last_selected_bed,
        loaded_snapshot.bed_selection_state.mode,
        loaded_snapshot.bed_selection_state.camera_action_on_selection
    );

    m_scene_interactor.set_state(
        project_id,
        std::move(loaded_snapshot.model),
        std::move(loaded_snapshot.object_selection)
    );

    m_scene_interactor.update_selection_bounding_box();

    ASSERT_VAL(m_gizmo_controller)->activate_tool(loaded_snapshot.selected_tool_gizmo);

    update_top_bar(project_id, it->second);
}

void Store::take_snapshot(Biz::UndoSnapshotType snapshot_type)
{
    if (!m_snapshotting_enabled) {
        return;
    }

    Domain::SelectionId project_id{m_project_interactor.selected_project_id()};
    const auto it{m_stacks.find(project_id)};
    if (it == m_stacks.end()) {
        return;
    }

    const Domain::Project& project{m_workbench.project(project_id)};

    const Scene::ToolType tool_type{
        m_gizmo_controller ? m_gizmo_controller->current_tool_type() :
                             Scene::ToolType::None,
    };

    it->second.take_snapshot(
        project.model(),
        m_scene_interactor.object_selection(project_id),
        tool_type,
        project.config_containers(),
        BedSelectionState{*m_scene_interactor.bed_selection(project_id)},
        snapshot_type
    );

    update_top_bar(project_id, it->second);
}

bool Store::is_undo_possible() const
{
    Domain::SelectionId project_id{m_project_interactor.selected_project_id()};
    const auto it{m_stacks.find(project_id)};
    if (it == m_stacks.end()) {
        return false;
    }
    return it->second.get_selected_index() >= 1;
}

bool Store::is_redo_possible() const
{
    Domain::SelectionId project_id{m_project_interactor.selected_project_id()};
    const auto it{m_stacks.find(project_id)};
    if (it == m_stacks.end()) {
        return false;
    }
    const std::optional<std::size_t> selected_index{it->second.get_selected_index()};
    const std::size_t size{it->second.get_snapshots().size()};
    if (size == 0) {
        return false;
    }
    return selected_index < size - 1;
}

void Store::on_project_loaded(Domain::SelectionId project_id)
{
    const bool inserted{m_stacks.emplace(project_id, Stack{}).second};
    ASSERT(inserted);

    take_snapshot(Biz::UndoSnapshotType::InitializeProject);
}

void Store::on_project_removed(Domain::SelectionId project_id)
{
    m_stacks.erase(project_id);
}

void Store::on_selected_project_changed_final(Domain::SelectionId project_id)
{
    if (!m_stacks.contains(project_id)) {
        return;
    }
    update_top_bar(project_id, m_stacks.at(project_id));
}

const Stack& Store::get_undo_stack(Domain::SelectionId project_id) const
{
    return m_stacks.at(project_id);
}

void Store::set_gizmo_controller(Scene::IGizmoController* gizmo_controller) {
    m_gizmo_controller = gizmo_controller;
}

void Store::update_top_bar(Domain::SelectionId project_id, const Stack& stack)
{
    std::vector<std::pair<std::size_t, std::string>> snapshots;
    snapshots.reserve(stack.get_snapshots().size());

    std::ranges::transform(
        stack.get_snapshots(),
        std::back_inserter(snapshots),
        [](const Snapshot& snapshot) { return std::pair{snapshot.id, to_string(snapshot.type)}; }
    );

    const std::optional<std::size_t> selected_index{stack.get_selected_index()};
    if (!selected_index) {
        return;
    }

    invoke_listeners<IStoreChangedListener>(
        [&](auto* listener)
        { listener->on_undo_store_changed(project_id, snapshots, *selected_index); }
    );
}
} // namespace Slic3r::App::Undo
