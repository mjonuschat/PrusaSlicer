#pragma once

#include <any>

namespace Slic3r::Domain {

enum class JobStatus
{
    None,
    Started,
    Finished,
    Failed
};

enum class JobProgressInfo
{
    None,
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

struct ProgressDetail
{
    JobProgressInfo info;
    std::any payload;
};

} // namespace Slic3r::Domain
