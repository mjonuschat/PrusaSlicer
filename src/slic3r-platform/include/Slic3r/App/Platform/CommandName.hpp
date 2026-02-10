#pragma once

namespace Slic3r::App::Platform {
struct CommandName
{
    static constexpr const char* AddObject      = "add-object";
    static constexpr const char* AddVolume      = "add-volume";
    static constexpr const char* AddInstance    = "add-instance";
    static constexpr const char* AddInstanceKp  = "add-instance-kp";
    static constexpr const char* DeleteSelected = "delete-selected";

    static constexpr const char* NewProject    = "new-project";
    static constexpr const char* OpenProject   = "open-project";
    static constexpr const char* SaveProject   = "save-project";
    static constexpr const char* SaveProjectAs = "save-project-as";

    static constexpr const char* ClearSelection = "clear-selection";

    static constexpr const char* MoveGizmo                  = "move-gizmo";
    static constexpr const char* RotateGizmo                = "rotate-gizmo";
    static constexpr const char* ScaleGizmo                 = "scale-gizmo";
    static constexpr const char* PlaceOnFace                = "place-on-face-gizmo";
    static constexpr const char* ArrangeGizmo               = "arrange-gizmo";
    static constexpr const char* SimplifyGizmo              = "simplify-gizmo";
    static constexpr const char* TextGizmo                  = "text-gizmo";
    static constexpr const char* CreateText                 = "create-text";
    static constexpr const char* CutGizmo                   = "cut-gizmo";
    static constexpr const char* MeasureGizmo               = "measure-gizmo";
    static constexpr const char* PaintOnSupportsGizmo       = "paint-on-supports-gizmo";
    static constexpr const char* PaintOnSeamsGizmo          = "paint-on-seams-gizmo";
    static constexpr const char* PaintOnFuzzySkinGizmo      = "paint-on-fuzzy-skin-gizmo";
    static constexpr const char* MultiMaterialPaintingGizmo = "multi-material-painting-gizmo";

    static constexpr const char* SwitchToPlater  = "switch-to-plater";
    static constexpr const char* SwitchToPreview = "switch-to-preview";
    static constexpr const char* SwitchToInspect = "switch-to-inspect";

    static constexpr const char* ShowTravels         = "show-travels";
    static constexpr const char* ShowWipes           = "show-wipes";
    static constexpr const char* ShowRetractions     = "show-retractions";
    static constexpr const char* ShowUnretractions   = "show-unretractions";
    static constexpr const char* ShowSeams           = "show-seams";
    static constexpr const char* ShowToolChanges     = "show-tool-changes";
    static constexpr const char* ShowColorChanges    = "show-color-changes";
    static constexpr const char* ShowPausePrints     = "show-pause-prints";
    static constexpr const char* ShowCustomGCodes    = "show-custom-g-codes";
    static constexpr const char* ShowCenterOfGravity = "show-center-of-gravity";
    static constexpr const char* ShowToolMarker      = "show-tool-marker";
    static constexpr const char* ShowShell           = "show-tool-shell";

    static constexpr const char* Search      = "search";
    static constexpr const char* Preferences = "preferences";
};
} // namespace Slic3r::App::Platform
