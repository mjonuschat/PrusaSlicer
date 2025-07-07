#/|/ Copyright (c) Prusa Research 2018 - 2023 Tomáš Mészáros @tamasmeszaros, Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Lukáš Hejl @hejllukas, Lukáš Matěna @lukasmatena, Filip Sykala @Jony01, David Kocík @kocikdav, Vojtěch Král @vojtechkral
#/|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
#/|/ Copyright (c) 2023 Mimoja @Mimoja
#/|/ Copyright (c) 2022 ole00 @ole00
#/|/
#/|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#/|/
cmake_minimum_required(VERSION 3.13)
project(libslic3r)

include(PrecompiledHeader)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/libslic3r/libslic3r_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/libslic3r/libslic3r_version.h @ONLY)

if(SLIC3R_STATIC)
    set(OPENVDB_USE_STATIC_LIBS ON)
    set(USE_BLOSC TRUE)
endif ()

find_package(OpenVDB 5.0 COMPONENTS openvdb)
if(OpenVDB_FOUND)
    slic3r_remap_configs(IlmBase::Half RelWithDebInfo Release)
    slic3r_remap_configs(Blosc::blosc RelWithDebInfo Release)
else ()
    message(FATAL_ERROR "OpenVDB could not be found with the bundled find module. "
                   "You can try to specify the find module location of your "
                   "OpenVDB installation with the OPENVDB_FIND_MODULE_PATH cache variable.")
endif ()

set(OpenVDBUtils_SOURCES "")
if (TARGET OpenVDB::openvdb)
    set(OpenVDBUtils_SOURCES
        src/libslic3r/OpenVDBUtils.cpp
        include/libslic3r/OpenVDBUtils.hpp
        src/libslic3r/OpenVDBUtilsLegacy.hpp
    )
endif ()

find_package(LibBGCode REQUIRED COMPONENTS Convert)
slic3r_remap_configs(LibBGCode::bgcode_core RelWithDebInfo Release)
slic3r_remap_configs(LibBGCode::bgcode_binarize RelWithDebInfo Release)
slic3r_remap_configs(LibBGCode::bgcode_convert RelWithDebInfo Release)

set(SLIC3R_SOURCES
    src/libslic3r/AABBMesh.cpp
    src/libslic3r/AABBTreeIndirect.hpp
    src/libslic3r/AABBTreeLines.hpp
    src/libslic3r/Algorithm/LineSegmentation/LineSegmentation.cpp
    src/libslic3r/Algorithm/LineSegmentation/LineSegmentation.hpp
    src/libslic3r/Algorithm/PathSorting.hpp
    src/libslic3r/Algorithm/RegionExpansion.cpp
    src/libslic3r/Algorithm/RegionExpansion.hpp
    src/libslic3r/AppConfig.cpp
    src/libslic3r/Arachne/BeadingStrategy/BeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/BeadingStrategy.hpp
    src/libslic3r/Arachne/BeadingStrategy/BeadingStrategyFactory.cpp
    src/libslic3r/Arachne/BeadingStrategy/BeadingStrategyFactory.hpp
    src/libslic3r/Arachne/BeadingStrategy/DistributedBeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/DistributedBeadingStrategy.hpp
    src/libslic3r/Arachne/BeadingStrategy/LimitedBeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/LimitedBeadingStrategy.hpp
    src/libslic3r/Arachne/BeadingStrategy/OuterWallInsetBeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/OuterWallInsetBeadingStrategy.hpp
    src/libslic3r/Arachne/BeadingStrategy/RedistributeBeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/RedistributeBeadingStrategy.hpp
    src/libslic3r/Arachne/BeadingStrategy/WideningBeadingStrategy.cpp
    src/libslic3r/Arachne/BeadingStrategy/WideningBeadingStrategy.hpp
    src/libslic3r/Arachne/PerimeterOrder.cpp
    src/libslic3r/Arachne/PerimeterOrder.hpp
    src/libslic3r/Arachne/SkeletalTrapezoidation.cpp
    src/libslic3r/Arachne/SkeletalTrapezoidation.hpp
    src/libslic3r/Arachne/SkeletalTrapezoidationEdge.hpp
    src/libslic3r/Arachne/SkeletalTrapezoidationGraph.cpp
    src/libslic3r/Arachne/SkeletalTrapezoidationGraph.hpp
    src/libslic3r/Arachne/SkeletalTrapezoidationJoint.hpp
    src/libslic3r/Arachne/utils/ExtrusionJunction.hpp
    src/libslic3r/Arachne/utils/ExtrusionLine.cpp
    src/libslic3r/Arachne/utils/ExtrusionLine.hpp
    src/libslic3r/Arachne/utils/HalfEdge.hpp
    src/libslic3r/Arachne/utils/HalfEdgeGraph.hpp
    src/libslic3r/Arachne/utils/HalfEdgeNode.hpp
    src/libslic3r/Arachne/utils/PolygonsPointIndex.hpp
    src/libslic3r/Arachne/utils/PolygonsSegmentIndex.hpp
    src/libslic3r/Arachne/utils/PolylineStitcher.cpp
    src/libslic3r/Arachne/utils/PolylineStitcher.hpp
    src/libslic3r/Arachne/utils/SparseGrid.hpp
    src/libslic3r/Arachne/utils/SparseLineGrid.hpp
    src/libslic3r/Arachne/utils/SparsePointGrid.hpp
    src/libslic3r/Arachne/utils/SquareGrid.cpp
    src/libslic3r/Arachne/utils/SquareGrid.hpp
    src/libslic3r/Arachne/WallToolPaths.cpp
    src/libslic3r/Arachne/WallToolPaths.hpp
#    src/libslic3r/ArrangeHelper.cpp
#    src/libslic3r/ArrangeHelper.hpp
    src/libslic3r/AStar.hpp
    src/libslic3r/BlacklistedLibraryCheck.cpp
    src/libslic3r/BlacklistedLibraryCheck.hpp
    src/libslic3r/BranchingTree/BranchingTree.cpp
    src/libslic3r/BranchingTree/BranchingTree.hpp
    src/libslic3r/BranchingTree/PointCloud.cpp
    src/libslic3r/BranchingTree/PointCloud.hpp
    src/libslic3r/BridgeDetector.cpp
    src/libslic3r/BridgeDetector.hpp
    src/libslic3r/Brim.cpp
    src/libslic3r/Brim.hpp
    src/libslic3r/BuildVolume.cpp
    src/libslic3r/BuildVolume.hpp
    src/libslic3r/ClipperUtils.cpp
    src/libslic3r/ClipperUtils.hpp
    src/libslic3r/ClipperZUtils.hpp
    src/libslic3r/Config.cpp
    src/libslic3r/ConfigPackUtils.cpp
    src/libslic3r/ConfigPackUtils.hpp
    src/libslic3r/ConfigUtils.cpp
    src/libslic3r/CSGMesh/CSGMeshCopy.hpp
    src/libslic3r/CSGMesh/ModelToCSGMesh.hpp
    src/libslic3r/CSGMesh/PerformCSGMeshBooleans.hpp
    src/libslic3r/CSGMesh/SliceCSGMesh.hpp
    src/libslic3r/CSGMesh/TriangleMeshAdapter.hpp
    src/libslic3r/CustomGCode.cpp
    src/libslic3r/CutUtils.cpp
    src/libslic3r/CutUtils.hpp
    src/libslic3r/EdgeGrid.cpp
    src/libslic3r/EdgeGrid.hpp
    src/libslic3r/ElephantFootCompensation.cpp
    src/libslic3r/ElephantFootCompensation.hpp
    src/libslic3r/Emboss.cpp
    src/libslic3r/Emboss.hpp
    src/libslic3r/ExPolygon.cpp
    src/libslic3r/ExPolygon.hpp
    src/libslic3r/ExPolygonSerialize.hpp
    src/libslic3r/ExPolygonsIndex.cpp
    src/libslic3r/ExPolygonsIndex.hpp
    src/libslic3r/Extruder.cpp
    src/libslic3r/Extruder.hpp
    src/libslic3r/ExtrusionEntity.cpp
    src/libslic3r/ExtrusionEntityCollection.cpp
    src/libslic3r/ExtrusionRole.cpp
    src/libslic3r/ExtrusionSimulator.cpp
    src/libslic3r/ExtrusionSimulator.hpp
    src/libslic3r/Feature/FuzzySkin/FuzzySkin.cpp
    src/libslic3r/Feature/FuzzySkin/FuzzySkin.hpp
    src/libslic3r/Feature/Interlocking/InterlockingGenerator.cpp
    src/libslic3r/Feature/Interlocking/InterlockingGenerator.hpp
    src/libslic3r/Feature/Interlocking/VoxelUtils.cpp
    src/libslic3r/Feature/Interlocking/VoxelUtils.hpp
    src/libslic3r/FileParserError.hpp
    src/libslic3r/Fill/Fill.cpp
    src/libslic3r/Fill/Fill3DHoneycomb.cpp
    src/libslic3r/Fill/Fill3DHoneycomb.hpp
    src/libslic3r/Fill/FillAdaptive.cpp
    src/libslic3r/Fill/FillBase.cpp
    src/libslic3r/Fill/FillConcentric.cpp
    src/libslic3r/Fill/FillConcentric.hpp
    src/libslic3r/Fill/FillEnsuring.cpp
    src/libslic3r/Fill/FillEnsuring.hpp
    src/libslic3r/Fill/FillGyroid.cpp
    src/libslic3r/Fill/FillGyroid.hpp
    src/libslic3r/Fill/FillHoneycomb.cpp
    src/libslic3r/Fill/FillHoneycomb.hpp
    src/libslic3r/Fill/FillLightning.cpp
    src/libslic3r/Fill/FillLine.cpp
    src/libslic3r/Fill/FillLine.hpp
    src/libslic3r/Fill/FillPlanePath.cpp
    src/libslic3r/Fill/FillPlanePath.hpp
    src/libslic3r/Fill/FillRectilinear.cpp
    src/libslic3r/Fill/FillRectilinear.hpp
    src/libslic3r/Fill/Lightning/DistanceField.cpp
    src/libslic3r/Fill/Lightning/DistanceField.hpp
    src/libslic3r/Fill/Lightning/Generator.cpp
    src/libslic3r/Fill/Lightning/Generator.hpp
    src/libslic3r/Fill/Lightning/Layer.cpp
    src/libslic3r/Fill/Lightning/Layer.hpp
    src/libslic3r/Fill/Lightning/TreeNode.cpp
    src/libslic3r/Fill/Lightning/TreeNode.hpp
    src/libslic3r/Flow.cpp
    src/libslic3r/Format/OBJ.cpp
    src/libslic3r/Format/OBJ.hpp
    src/libslic3r/Format/objparser.cpp
    src/libslic3r/Format/objparser.hpp
    src/libslic3r/Format/PrintRequest.cpp
    src/libslic3r/Format/PrintRequest.cpp
    src/libslic3r/Format/SL1.cpp
    src/libslic3r/Format/SL1.hpp
    src/libslic3r/Format/SL1_SVG.cpp
    src/libslic3r/Format/SL1_SVG.hpp
    src/libslic3r/Format/STEP.cpp
    src/libslic3r/Format/STEP.hpp
    src/libslic3r/Format/STL.cpp
    src/libslic3r/Format/STL.hpp
    src/libslic3r/Format/SVG.cpp
    src/libslic3r/Format/SVG.hpp
    src/libslic3r/GCode.cpp
    src/libslic3r/GCode.hpp
    src/libslic3r/GCode/AvoidCrossingPerimeters.cpp
    src/libslic3r/GCode/AvoidCrossingPerimeters.hpp
    src/libslic3r/GCode/ConflictChecker.cpp
    src/libslic3r/GCode/ConflictChecker.hpp
    src/libslic3r/GCode/CoolingBuffer.cpp
    src/libslic3r/GCode/CoolingBuffer.hpp
    src/libslic3r/GCode/ExtrusionOrder.cpp
    src/libslic3r/GCode/ExtrusionOrder.hpp
    src/libslic3r/GCode/ExtrusionProcessor.cpp
    src/libslic3r/GCode/ExtrusionProcessor.hpp
    src/libslic3r/GCode/FindReplace.cpp
    src/libslic3r/GCode/FindReplace.hpp
    src/libslic3r/GCode/GCodeWriter.cpp
    src/libslic3r/GCode/GCodeWriter.hpp
    src/libslic3r/GCode/LabelObjects.cpp
    src/libslic3r/GCode/LabelObjects.hpp
    src/libslic3r/GCode/ModelVisibility.cpp
    src/libslic3r/GCode/ModelVisibility.hpp
    src/libslic3r/GCode/PostProcessor.cpp
    src/libslic3r/GCode/PostProcessor.hpp
    src/libslic3r/GCode/PressureEqualizer.cpp
    src/libslic3r/GCode/PressureEqualizer.hpp
    src/libslic3r/GCode/PrintExtents.cpp
    src/libslic3r/GCode/PrintExtents.hpp
    src/libslic3r/GCode/RetractWhenCrossingPerimeters.cpp
    src/libslic3r/GCode/RetractWhenCrossingPerimeters.hpp
    src/libslic3r/GCode/SeamAligned.cpp
    src/libslic3r/GCode/SeamAligned.hpp
    src/libslic3r/GCode/SeamChoice.cpp
    src/libslic3r/GCode/SeamChoice.hpp
    src/libslic3r/GCode/SeamGeometry.cpp
    src/libslic3r/GCode/SeamGeometry.hpp
    src/libslic3r/GCode/SeamPainting.cpp
    src/libslic3r/GCode/SeamPainting.hpp
    src/libslic3r/GCode/SeamPerimeters.cpp
    src/libslic3r/GCode/SeamPerimeters.hpp
    src/libslic3r/GCode/SeamPlacer.cpp
    src/libslic3r/GCode/SeamPlacer.hpp
    src/libslic3r/GCode/SeamRandom.cpp
    src/libslic3r/GCode/SeamRandom.hpp
    src/libslic3r/GCode/SeamRear.cpp
    src/libslic3r/GCode/SeamRear.hpp
    src/libslic3r/GCode/SeamScarf.cpp
    src/libslic3r/GCode/SeamScarf.hpp
    src/libslic3r/GCode/SeamShells.cpp
    src/libslic3r/GCode/SeamShells.hpp
    src/libslic3r/GCode/SmoothPath.cpp
    src/libslic3r/GCode/SmoothPath.hpp
    src/libslic3r/GCode/SpiralVase.cpp
    src/libslic3r/GCode/SpiralVase.hpp
    src/libslic3r/GCode/ThumbnailData.cpp
    src/libslic3r/GCode/Thumbnails.cpp
    src/libslic3r/GCode/ToolOrdering.cpp
    src/libslic3r/GCode/Travels.cpp
    src/libslic3r/GCode/Travels.hpp
    src/libslic3r/GCode/Wipe.cpp
    src/libslic3r/GCode/Wipe.hpp
    src/libslic3r/GCode/WipeTower.cpp
    src/libslic3r/GCode/WipeTowerIntegration.cpp
    src/libslic3r/GCode/WipeTowerIntegration.hpp
    src/libslic3r/Geometry/ArcWelder.cpp
    src/libslic3r/Geometry/ArcWelder.hpp
    src/libslic3r/Geometry/Bicubic.hpp
    src/libslic3r/Geometry/Circle.cpp
    src/libslic3r/Geometry/Circle.hpp
    src/libslic3r/Geometry/ConvexHull.hpp
    src/libslic3r/Geometry/Curves.hpp
    src/libslic3r/Geometry/MedialAxis.cpp
    src/libslic3r/Geometry/MedialAxis.hpp
    src/libslic3r/Geometry/Voronoi.cpp
    src/libslic3r/Geometry/Voronoi.hpp
    src/libslic3r/Geometry/VoronoiOffset.cpp
    src/libslic3r/Geometry/VoronoiOffset.hpp
    src/libslic3r/Geometry/VoronoiUtils.cpp
    src/libslic3r/Geometry/VoronoiUtils.cpp
    src/libslic3r/Geometry/VoronoiUtils.hpp
    src/libslic3r/Geometry/VoronoiUtils.hpp
    src/libslic3r/Geometry/VoronoiVisualUtils.hpp
    src/libslic3r/InfillAboveBridges.cpp
    src/libslic3r/InfillAboveBridges.hpp
    src/libslic3r/JumpPointSearch.cpp
    src/libslic3r/JumpPointSearch.hpp
    src/libslic3r/KDTreeIndirect.hpp
    src/libslic3r/Layer.cpp
    src/libslic3r/Layer.hpp
    src/libslic3r/LayerRegion.cpp
    src/libslic3r/LayerRegion.hpp
    src/libslic3r/Line.hpp
    src/libslic3r/MarchingSquares.hpp
    src/libslic3r/Measure.cpp
    src/libslic3r/Measure.hpp
    src/libslic3r/MeasureUtils.hpp
    src/libslic3r/MeshNormals.cpp
    src/libslic3r/MeshNormals.hpp
    src/libslic3r/MinAreaBoundingBox.cpp
    src/libslic3r/miniz_extension.cpp
    src/libslic3r/Model.cpp
    src/libslic3r/Model.hpp
    src/libslic3r/ModelProcessing.cpp
    src/libslic3r/ModelProcessing.hpp
    src/libslic3r/ModelUtils.cpp
    src/libslic3r/MTUtils.hpp
    src/libslic3r/MultiMaterialSegmentation.cpp
    src/libslic3r/MultiMaterialSegmentation.hpp
    src/libslic3r/MultipleBeds.cpp
    src/libslic3r/MultiPoint.cpp
    src/libslic3r/MultiPoint.hpp
    src/libslic3r/MutablePolygon.cpp
    src/libslic3r/MutablePolygon.hpp
    src/libslic3r/MutablePriorityQueue.hpp
    src/libslic3r/NormalUtils.cpp
    src/libslic3r/NormalUtils.hpp
    src/libslic3r/NSVGUtils.cpp
    src/libslic3r/Optimize/BruteforceOptimizer.hpp
    src/libslic3r/PerimeterGenerator.cpp
    src/libslic3r/PerimeterGenerator.hpp
    src/libslic3r/PNGReadWrite.cpp
    src/libslic3r/PNGReadWrite.hpp
    src/libslic3r/Point.cpp
    src/libslic3r/Point.hpp
    src/libslic3r/PointGrid.hpp
    src/libslic3r/Polygon.cpp
    src/libslic3r/Polygon.hpp
    src/libslic3r/PolygonTrimmer.cpp
    src/libslic3r/PolygonTrimmer.hpp
    src/libslic3r/Polyline.cpp
    src/libslic3r/Polyline.hpp
    src/libslic3r/PrePreview.cpp
    src/libslic3r/PrePreview.hpp
    src/libslic3r/Preset.cpp
    src/libslic3r/PresetBundle.cpp
    src/libslic3r/PrincipalComponents2D.cpp
    src/libslic3r/PrincipalComponents2D.hpp
    src/libslic3r/Print.cpp
    src/libslic3r/PrintApply.cpp
    src/libslic3r/PrintBase.cpp
    src/libslic3r/PrintConfig.cpp
    src/libslic3r/PrintObject.cpp
    src/libslic3r/PrintObjectSlice.cpp
    src/libslic3r/PrintRegion.cpp
    src/libslic3r/ShortEdgeCollapse.cpp
    src/libslic3r/ShortEdgeCollapse.hpp
    src/libslic3r/ShortestPath.cpp
    src/libslic3r/ShortestPath.hpp
    src/libslic3r/SLA/AGGRaster.hpp
    src/libslic3r/SLA/BranchingTreeSLA.cpp
    src/libslic3r/SLA/BranchingTreeSLA.hpp
    src/libslic3r/SLA/Clustering.cpp
    src/libslic3r/SLA/Clustering.hpp
    src/libslic3r/SLA/ConcaveHull.cpp
    src/libslic3r/SLA/ConcaveHull.hpp
    src/libslic3r/SLA/DefaultSupportTree.cpp
    src/libslic3r/SLA/DefaultSupportTree.hpp
    src/libslic3r/SLA/Hollowing.cpp
    src/libslic3r/SLA/Pad.cpp
    src/libslic3r/SLA/RasterBase.cpp
    src/libslic3r/SLA/RasterBase.hpp
    src/libslic3r/SLA/RasterToPolygons.cpp
    src/libslic3r/SLA/RasterToPolygons.hpp
    src/libslic3r/SLA/ReprojectPointsOnMesh.hpp
    src/libslic3r/SLA/Rotfinder.cpp
    src/libslic3r/SLA/Rotfinder.hpp
    src/libslic3r/SLA/SpatIndex.cpp
    src/libslic3r/SLA/SpatIndex.hpp
    src/libslic3r/SLA/SupportIslands/EvaluateNeighbor.cpp
    src/libslic3r/SLA/SupportIslands/EvaluateNeighbor.hpp
    src/libslic3r/SLA/SupportIslands/ExpandNeighbor.cpp
    src/libslic3r/SLA/SupportIslands/ExpandNeighbor.hpp
    src/libslic3r/SLA/SupportIslands/IStackFunction.hpp
    src/libslic3r/SLA/SupportIslands/LineUtils.cpp
    src/libslic3r/SLA/SupportIslands/LineUtils.hpp
    src/libslic3r/SLA/SupportIslands/NodeDataWithResult.hpp
    src/libslic3r/SLA/SupportIslands/Parabola.hpp
    src/libslic3r/SLA/SupportIslands/ParabolaUtils.cpp
    src/libslic3r/SLA/SupportIslands/ParabolaUtils.hpp
    src/libslic3r/SLA/SupportIslands/PointUtils.cpp
    src/libslic3r/SLA/SupportIslands/PointUtils.hpp
    src/libslic3r/SLA/SupportIslands/PolygonUtils.cpp
    src/libslic3r/SLA/SupportIslands/PolygonUtils.hpp
    src/libslic3r/SLA/SupportIslands/PostProcessNeighbor.cpp
    src/libslic3r/SLA/SupportIslands/PostProcessNeighbor.hpp
    src/libslic3r/SLA/SupportIslands/PostProcessNeighbors.cpp
    src/libslic3r/SLA/SupportIslands/PostProcessNeighbors.hpp
    src/libslic3r/SLA/SupportIslands/SampleConfigFactory.cpp
    src/libslic3r/SLA/SupportIslands/SampleConfigFactory.hpp
    src/libslic3r/SLA/SupportIslands/SupportIslandPoint.cpp
    src/libslic3r/SLA/SupportIslands/SupportIslandPoint.hpp
    src/libslic3r/SLA/SupportIslands/UniformSupportIsland.cpp
    src/libslic3r/SLA/SupportIslands/UniformSupportIsland.hpp
    src/libslic3r/SLA/SupportIslands/VectorUtils.hpp
    src/libslic3r/SLA/SupportIslands/VoronoiGraph.hpp
    src/libslic3r/SLA/SupportIslands/VoronoiGraphUtils.cpp
    src/libslic3r/SLA/SupportIslands/VoronoiGraphUtils.hpp
    src/libslic3r/SLA/SupportPointGenerator.cpp
    src/libslic3r/SLA/SupportTree.cpp
    src/libslic3r/SLA/SupportTreeBuilder.cpp
    src/libslic3r/SLA/SupportTreeBuilder.hpp
    src/libslic3r/SLA/SupportTreeMesher.cpp
    src/libslic3r/SLA/SupportTreeMesher.hpp
    src/libslic3r/SLA/SupportTreeUtils.hpp
    src/libslic3r/SLA/SupportTreeUtilsLegacy.hpp
    src/libslic3r/SLA/ZCorrection.cpp
    src/libslic3r/SLA/ZCorrection.hpp
    src/libslic3r/SLAPrint.cpp
    src/libslic3r/SLAPrintSteps.cpp
    src/libslic3r/SLAPrintSteps.hpp
    src/libslic3r/SlicesToTriangleMesh.cpp
    src/libslic3r/SlicesToTriangleMesh.hpp
    src/libslic3r/Slicing.cpp
    src/libslic3r/SlicingAdaptive.cpp
    src/libslic3r/SlicingAdaptive.hpp
    src/libslic3r/SlicingInput.cpp
    src/libslic3r/SlicingInput.hpp
    src/libslic3r/StepsInvalidation.cpp
    src/libslic3r/StepsInvalidation.hpp
    src/libslic3r/Subdivide.cpp
    src/libslic3r/Subdivide.hpp
    src/libslic3r/Support/OrganicSupport.cpp
    src/libslic3r/Support/OrganicSupport.hpp
    src/libslic3r/Support/SupportCommon.cpp
    src/libslic3r/Support/SupportCommon.hpp
    src/libslic3r/Support/SupportDebug.cpp
    src/libslic3r/Support/SupportDebug.hpp
    src/libslic3r/Support/SupportLayer.hpp
    src/libslic3r/Support/SupportMaterial.cpp
    src/libslic3r/Support/SupportMaterial.hpp
    src/libslic3r/Support/SupportParameters.cpp
    src/libslic3r/Support/SupportParameters.hpp
    src/libslic3r/Support/TreeModelVolumes.cpp
    src/libslic3r/Support/TreeModelVolumes.hpp
    src/libslic3r/Support/TreeSupport.cpp
    src/libslic3r/Support/TreeSupport.hpp
    src/libslic3r/Support/TreeSupportCommon.cpp
    src/libslic3r/Support/TreeSupportCommon.hpp
    src/libslic3r/SupportSpotsGenerator.cpp
    src/libslic3r/Surface.cpp
    src/libslic3r/Surface.hpp
    src/libslic3r/SurfaceCollection.cpp
    src/libslic3r/SurfaceCollection.hpp
    src/libslic3r/SurfaceMesh.hpp
    src/libslic3r/Technologies.hpp
    src/libslic3r/Thread.cpp
    src/libslic3r/Thread.hpp
    src/libslic3r/TriangleMeshSlicer.cpp
    src/libslic3r/TriangleSelectorWrapper.cpp
    src/libslic3r/TriangleSelectorWrapper.hpp
    src/libslic3r/TriangleSetSampling.cpp
    src/libslic3r/TriangleSetSampling.hpp
    src/libslic3r/TriangulateWall.hpp
    src/libslic3r/utils.cpp
    src/libslic3r/Utils/DirectoriesUtils.cpp
    src/libslic3r/Utils/JsonUtils.cpp
    src/libslic3r/Utils/JsonUtils.hpp
    src/libslic3r/Zipper.cpp
    src/libslic3r/Zipper.hpp

    src/libslic3r/libslic3r.h
    src/libslic3r/pchheader.cpp
    src/libslic3r/pchheader.hpp

    ${OpenVDBUtils_SOURCES}
)

if (APPLE)
    list(APPEND SLIC3R_SOURCES
        src/libslic3r/MacUtils.mm
    )
endif ()

set(SLIC3R_PUBLIC_HEADERS
    include/libslic3r/AABBMesh.hpp
    include/libslic3r/AnyPtr.hpp
    include/libslic3r/AppConfig.hpp
    include/libslic3r/BoostAdapter.hpp
    include/libslic3r/clonable_ptr.hpp
    include/libslic3r/Config.hpp
    include/libslic3r/ConfigUtils.hpp
    include/libslic3r/ConfigViews.hpp
    include/libslic3r/CSGMesh/CSGMesh.hpp
    include/libslic3r/CSGMesh/VoxelizeCSGMesh.hpp
    include/libslic3r/CustomGCode.hpp
    include/libslic3r/ExtrusionEntity.hpp
    include/libslic3r/ExtrusionEntityCollection.hpp
    include/libslic3r/ExtrusionRole.hpp
    include/libslic3r/Fill/FillAdaptive.hpp
    include/libslic3r/Fill/FillBase.hpp
    include/libslic3r/Fill/FillLightning.hpp
    include/libslic3r/Flow.hpp
    include/libslic3r/format.hpp
    include/libslic3r/GCode/ThumbnailData.hpp
    include/libslic3r/GCode/Thumbnails.hpp
    include/libslic3r/GCode/ToolOrdering.hpp
    include/libslic3r/GCode/WipeTower.hpp
    include/libslic3r/Geometry.hpp
    include/libslic3r/MeshBoolean.hpp
    include/libslic3r/MinAreaBoundingBox.hpp
    include/libslic3r/miniz_extension.hpp
    include/libslic3r/ModelUtils.hpp
    include/libslic3r/MultipleBeds.hpp
    include/libslic3r/NSVGUtils.hpp
    include/libslic3r/OpenVDBUtils.hpp
    include/libslic3r/Optimize/NLoptOptimizer.hpp
    include/libslic3r/Optimize/Optimizer.hpp
    include/libslic3r/Preset.hpp
    include/libslic3r/PresetBundle.hpp
    include/libslic3r/Print.hpp
    include/libslic3r/PrintBase.hpp
    include/libslic3r/PrintConfig.hpp
    include/libslic3r/SerializedConfig.hpp
    include/libslic3r/SLA/Hollowing.hpp
    include/libslic3r/SLA/JobController.hpp
    include/libslic3r/SLA/Pad.hpp
    include/libslic3r/SLA/SLAResult.hpp
    include/libslic3r/SLA/SupportIslands/SampleConfig.hpp
    include/libslic3r/SLA/SupportPointGenerator.hpp
    include/libslic3r/SLA/SupportTree.hpp
    include/libslic3r/SLA/SupportTreeStrategies.hpp
    include/libslic3r/SLAPrint.hpp
    include/libslic3r/Slicing.hpp
    include/libslic3r/StaticMap.hpp
    include/libslic3r/SupportSpotsGenerator.hpp
    include/libslic3r/TriangleMeshSlicer.hpp
    include/libslic3r/Utils.hpp
    include/libslic3r/Utils/DirectoriesUtils.hpp

    "${CMAKE_CURRENT_BINARY_DIR}/include/libslic3r/libslic3r_version.h"
)

add_library(libslic3r STATIC ${SLIC3R_SOURCES} ${SLIC3R_PUBLIC_HEADERS})

if (WIN32)
    target_compile_definitions(libslic3r PUBLIC NOMINMAX)
endif ()

foreach(_source IN ITEMS ${SLIC3R_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_LIST_SLA})

if (SLIC3R_STATIC)
    set(CGAL_Boost_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
endif ()
set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE ON CACHE BOOL "" FORCE)

cmake_policy(PUSH)
cmake_policy(SET CMP0011 NEW)
find_package(CGAL REQUIRED)
cmake_policy(POP)

add_library(libslic3r_cgal STATIC
    src/libslic3r/CutSurface.hpp
    src/libslic3r/CutSurface.cpp
    src/libslic3r/Geometry/VoronoiUtilsCgal.hpp
    src/libslic3r/Geometry/VoronoiUtilsCgal.cpp
    src/libslic3r/IntersectionPoints.hpp
    src/libslic3r/IntersectionPoints.cpp
    src/libslic3r/MeshBoolean.cpp
    src/libslic3r/TryCatchSignal.hpp
    src/libslic3r/TryCatchSignal.cpp
    src/libslic3r/Triangulation.hpp
    src/libslic3r/Triangulation.cpp
    src/libslic3r/SLA/SupportIslands/VoronoiDiagramCGAL.hpp
    src/libslic3r/SLA/SupportIslands/VoronoiDiagramCGAL.cpp

    include/libslic3r/MeshBoolean.hpp
)

target_include_directories(libslic3r_cgal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Reset compile options of libslic3r_cgal. Despite it being linked privately, CGAL options
# (-frounding-math) still propagate to dependent libs which is not desired.
get_target_property(_cgal_tgt CGAL::CGAL ALIASED_TARGET)
if (NOT TARGET ${_cgal_tgt})
    set (_cgal_tgt CGAL::CGAL)
endif ()
get_target_property(_opts ${_cgal_tgt} INTERFACE_COMPILE_OPTIONS)
if (_opts)
    set(_opts_bad "${_opts}")
    set(_opts_good "${_opts}")
    list(FILTER _opts_bad INCLUDE REGEX frounding-math)
    list(FILTER _opts_good EXCLUDE REGEX frounding-math)
    set_target_properties(${_cgal_tgt} PROPERTIES INTERFACE_COMPILE_OPTIONS "${_opts_good}")
    target_compile_options(libslic3r_cgal PRIVATE "${_opts_bad}")
endif()

target_link_libraries(libslic3r_cgal PRIVATE ${_cgal_tgt} libigl)
target_link_libraries(libslic3r_cgal PUBLIC
    admesh
    localesutils
    slic3r-domain
    slic3r-biz-algorithms
    clipper
    clipper_z
    libslic3r
)

if (MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL "4") # 32 bit MSVC workaround
    target_compile_definitions(libslic3r_cgal PRIVATE CGAL_DO_NOT_USE_MPZF)
endif ()

target_compile_definitions(libslic3r PUBLIC -DUSE_TBB -DTBB_USE_CAPTURED_EXCEPTION=0)

target_include_directories(libslic3r PUBLIC include)
target_include_directories(libslic3r PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include) # For libslic3r_version.h
target_include_directories(libslic3r PRIVATE src)

find_package(JPEG REQUIRED)

target_link_libraries(libslic3r PRIVATE
    libnest2d
    libcereal
    boost_libs
    libexpat
    qhull
    TBB::tbb
    TBB::tbbmalloc
    libslic3r_cgal
    ${CMAKE_DL_LIBS}
    JPEG::JPEG
    qoi
    fastfloat
    int128
    spdlog
)

if (NOT EMSCRIPTEN)
    target_link_libraries(libslic3r PRIVATE
            PNG::PNG
            ZLIB::ZLIB
            JPEG::JPEG
    )
else ()
    set(CMAKE_C_FLAGS "--use-port=libpng --use-port=zlib ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "--use-port=libpng --use-port=zlib ${CMAKE_CXX_FLAGS}")
endif ()

target_link_libraries(libslic3r PUBLIC
    admesh
    localesutils
    LibBGCode::bgcode_convert
    tcbspan
    miniz
    libigl
    agg
    ankerl
    boost_headeronly
    #libseqarrange
    slic3r-jthread
    slic3r-domain
    slic3r-gcode-reader
    slic3r-biz-algorithms
    libpgcode
    clipper
    clipper_z
    slic3r-biz-parser
)

if (APPLE)
    # TODO: we need to fix notarization with the separate shared library
    target_link_libraries(libslic3r PUBLIC OCCTWrapper)
endif ()

if (TARGET OpenVDB::openvdb)
    target_link_libraries(libslic3r PUBLIC OpenVDB::openvdb Blosc::blosc)
endif()

if(WIN32)
    target_link_libraries(libslic3r PUBLIC Psapi.lib)
endif()

if (APPLE)
    # This flag prevents the need for minimum SDK version 10.14
    # currently, PS targets v10.12
    target_compile_options(libslic3r PUBLIC "-fno-aligned-allocation")
    target_compile_options(libslic3r_cgal PUBLIC "-fno-aligned-allocation")
endif ()

if (EMSCRIPTEN)
    # Emscripten has no support for rounding FP env
    # For discussion see:
    # https://github.com/emscripten-core/emscripten/issues/11086
    # https://github.com/WebAssembly/relaxed-simd/issues/19
    target_compile_definitions(libslic3r_cgal PUBLIC -DCGAL_ALWAYS_ROUND_TO_NEAREST)
endif ()

if (SLIC3R_PCH AND NOT SLIC3R_SYNTAXONLY)
    add_precompiled_header(libslic3r src/libslic3r/pchheader.hpp FORCEINCLUDE)
endif ()

#add_target_to_doxygen(libslic3r INPUT .)
