#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Domain/GCodeFlavor.hpp"


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
enum class SlicingMode
{    
    Regular, // Regular, applying ClipperLib::pftNonZero rule when creating ExPolygons.  
    EvenOdd, // Compatible with 3DLabPrint models, applying ClipperLib::pftEvenOdd rule when creating ExPolygons.
    CloseHoles, // Orienting all contours CCW, thus closing all holes.
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
    PrintSettings() : ConfigBox(s_defs_fdm, "print_settings") {}
};
class FilamentSettings : public ConfigBox
{
public:
    FilamentSettings() : ConfigBox(s_defs_fdm, "filament_settings") {}
};
class PrinterSettings : public ConfigBox
{
public:
    PrinterSettings() : ConfigBox(s_defs_fdm, "printer_settings") {}
};
class ToolPrintSettings : public ConfigBox
{
public:
    ToolPrintSettings() : ConfigBox(s_defs_fdm, "toolprint_settings") {}
};
class ObjectSettings : public ConfigBox
{
public:
    ObjectSettings() : ConfigBox(s_defs_fdm, "object_settings") {}
};
class VolumeSettings : public ConfigBox
{
public:
    VolumeSettings() : ConfigBox(s_defs_fdm, "volume_settings") {}
};
class ProjectSettings : public ConfigBox
{
public:
    ProjectSettings() : ConfigBox(s_defs_fdm, "project_settings") {}
};


class FullConfigFDM : public FullConfig
{
public:
    FullConfigFDM(const PrinterSettings& printer_s,
                  const std::vector<std::reference_wrapper<const FilamentSettings>>& filament_s,
                  const PrintSettings& print_s,
                  const std::vector<std::reference_wrapper<const ToolPrintSettings>>& tool_print_s,
                  const ProjectSettings& project_s)
    {
        ASSERT(filament_s.size() == tool_print_s.size());

        // The base class only gets base pointers to base ConfigBoxes - it needs not to know what they are.
        add(printer_s);
        add(print_s);
        std::vector<std::reference_wrapper<const ConfigBox>> tps_s(tool_print_s.begin(), tool_print_s.end());
        add(tps_s);
        std::vector<std::reference_wrapper<const ConfigBox>> fil_s(filament_s.begin(), filament_s.end());
        add(fil_s);
        add(project_s);
    }

    std::string_view name() const override { return "FDM"; }
};

} // namespace Slic3r::Domain
