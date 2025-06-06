#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/GCodeFlavor.hpp"
#include "Slic3r/Domain/ConfigCommon.hpp"


namespace Slic3r::Domain {

// This is an example of using Config infrastructure.

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
extern ConfigDefinitions s_defs_fdm;

// Next, define all enums that should be used in the config.

enum class ArcFittingType {
    Disabled,
    EmitCenter
};
enum class TopOnePerimeterType
{
    None,
    TopSurfaces,
    TopmostOnly,
    Count
};
enum class BrimType {
    NoBrim,
    OuterOnly,
    InnerOnly,
    OuterAndInner,
};
enum class EnsureVerticalShellThickness {
    Disabled,
    Partial,
    Enabled,
};
enum class InfillPattern {
    ipRectilinear, ipMonotonic, ipMonotonicLines, ipAlignedRectilinear, ipGrid, ipTriangles,
    ipStars, ipCubic, ipLine, ipConcentric, ipHoneycomb, ip3DHoneycomb, ipGyroid, ipHilbertCurve,
    ipArchimedeanChords, ipOctagramSpiral, ipAdaptiveCubic, ipSupportCubic, ipSupportBase,
    ipLightning, ipEnsuring, ipZigZag, ipCount,
};
enum class FuzzySkinType {
    None,
    External,
    All,
};


//This is defined in a separate file:
// enum class GCodeFlavor

enum class LabelObjectsStyle {
    Disabled, Octoprint, Firmware
};
enum class IroningType {
    TopSurfaces,
    TopmostOnly,
    AllSolid,
    Count,
};
enum class MachineLimitsUsage {
    EmitToGCode,
    TimeEstimateOnly,
    Ignore,
    Count,
};
enum class SeamPosition {
    spRandom, spNearest, spAligned, spRear
};
enum class ScarfSeamPlacement {
    nowhere,
    countours,
    everywhere
};
enum class DraftShield {
    dsDisabled, dsLimited, dsEnabled
};
enum class SupportMaterialPattern {
    smpRectilinear, smpRectilinearGrid, smpHoneycomb,
};
enum class SupportMaterialInterfacePattern {
    smipAuto, smipRectilinear, smipConcentric,
};
enum class SupportMaterialStyle {
    smsGrid, smsSnug, smsTree, smsOrganic,
};
enum class PerimeterGeneratorType
{
    Classic, // Classic perimeter generator using Clipper offsets with constant extrusion width.
    Arachne  // Perimeter generator with variable extrusion width based on the paper  "A framework for
             // "adaptive width control of dense contour-parallel toolpaths in fused deposition modeling" ported from Cura.
};

// Then, define all types of ConfigBoxes that will be used. Provide our list
// of definitions and the type of the box (which must match definitions).

class PrintSettings : public ConfigBox
{
public:
    PrintSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Print) {}
};
class FilamentSettings : public ConfigBox
{
public:
    FilamentSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Filament) {}
};
class PrinterSettings : public ConfigBox
{
public:
    PrinterSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Printer) {}
};
class ToolPrintSettings : public ConfigBox
{
public:
    ToolPrintSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Tool) {}
};
class ObjectSettings : public ConfigBox
{
public:
    ObjectSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Object) {}
};
class VolumeSettings : public ConfigBox
{
public:
    VolumeSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Volume) {}
};
class ProjectSettings : public ConfigBox
{
public:
    ProjectSettings() : ConfigBox(s_defs_fdm, FDMConfigLocation::Project) {}
};

} // namespace Slic3r::Domain
