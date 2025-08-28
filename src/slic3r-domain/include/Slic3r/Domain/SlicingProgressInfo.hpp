#pragma once

namespace Slic3r::Domain {

enum class SlicingProgressInfo : int
{
    None = 0,
    SlicingProgressGeneratingWipeTower,
    SlicingProgressGeneratingSkirtAndBrim,
    SlicingProgressGeneratingPerimeters,
    SlicingProgressPreparingInfill,
    SlicingProgressMakingInfill,
    SlicingProgressSearchingSupportSpots,
    SlicingProgressGeneratingSupportMaterial,
    SlicingProgressEstimatingCurledExtrusions,
    SlicingProgressCalculatingOverhangingPerimeters,
    SlicingProgressProcessingTriangulatedMesh,
};

} // namespace Slic3r::Domain
