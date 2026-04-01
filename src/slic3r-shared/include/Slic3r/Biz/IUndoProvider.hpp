#pragma once

#include <variant>

namespace Slic3r::Biz {

enum class UndoSnapshotType
{
    None,
    InitializeProject,
    QuickDrag,
    Translate,
    SetTranslation,
    Rotate,
    SetRotation,
    RevertRotation,
    Scale,
    SetScale,
    RevertScale,
    PlaceOnFace,
    Arrange,
    Cut,
    AddObject,
    AddInstance,
    AddVolume,
    AddVolumeCube,
    AddVolumeCylinder,
    AddVolumeSphere,
    AddVolumeText,
    AddVolumeSvg,
    AddVolumeGallery,
    AddNegativeVolume,
    AddNegativeVolumeCube,
    AddNegativeVolumeCylinder,
    AddNegativeVolumeSphere,
    AddNegativeVolumeText,
    AddNegativeVolumeSvg,
    AddNegativeVolumeGallery,
    AddModifier,
    AddModifierCube,
    AddModifierCylinder,
    AddModifierSphere,
    AddModifierText,
    AddModifierSvg,
    AddModifierGallery,
    AddSupportBlocker,
    AddSupportBlockerCube,
    AddSupportBlockerCylinder,
    AddSupportBlockerSphere,
    AddSupportBlockerGallery,
    AddSupportModifier,
    AddSupportModifierCube,
    AddSupportModifierCylinder,
    AddSupportModifierSphere,
    AddSupportModifierGallery,
    AddCube,
    AddCylinder,
    AddSphere,
    DeleteSelection,
    PaintOnSupportsStroke,
    PaintOnSeamsStroke,
    PaintOnFuzzySkinStroke,
    MMPaintingStroke,
    ActivateGizmo,
    DeactivateGizmo,
    SelectBed,
    AddConfigContainer,
    DeleteConfigContainer,
    AddBed,
    DeleteBed,
    SelectPrinterPreset,
    SetPartSettingsValue
};

namespace UndoSnapshotSelection {
struct Next
{};

struct Prev
{};

using Variant = std::variant<std::size_t, Next, Prev>;
} // namespace UndoSnapshotSelection

class IUndoProvider
{
public:
    virtual ~IUndoProvider() = default;

    virtual void take_snapshot(UndoSnapshotType type) = 0;

    virtual void select_snapshot(UndoSnapshotSelection::Variant snapshot_variant) = 0;

    virtual bool is_undo_possible() const = 0;

    virtual bool is_redo_possible() const = 0;
};

} // namespace Slic3r::Biz
