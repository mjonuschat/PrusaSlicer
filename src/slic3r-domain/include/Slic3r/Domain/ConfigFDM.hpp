#pragma once

#include "Slic3r/Domain/Config.hpp"


// This is an example of using Config infrastructure.

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
extern ConfigDefinitions s_defs_fdm;



// Next, define all enums that should be used in the config.
enum class PrinterTechnology
{
    FFF,
    SLA
};
enum class GCodeThumbnailsFormat {
    PNG, JPG, QOI
};
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
enum class GCodeFlavor {
    gcfRepRapSprinter,
    gcfRepRapFirmware,
    gcfRepetier,
    gcfTeacup,
    gcfMakerWare,
    gcfMarlinLegacy,
    gcfMarlinFirmware,
    gcfKlipper,
    gcfSailfish,
    gcfMach3,
    gcfMachinekit,
    gcfSmoothie,
    gcfNoExtrusion,
};
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


class FullConfigFDM : public FullConfig
{
public:
    FullConfigFDM(const PrinterSettings& printer_s,
                  const std::vector<FilamentSettings*>& filament_s,
                  const PrintSettings& print_s,
                  const std::vector<ToolPrintSettings*>& tool_print_s)
    : m_printer_settings(printer_s), m_print_settings(print_s)
    {
        

        // Create copies of the boxes.
        for (const FilamentSettings* fs : filament_s)
            m_filament_settings.emplace_back(*fs);
        for (const ToolPrintSettings* tps : tool_print_s)
            m_tool_print_settings.emplace_back(*tps);

        // Create vectors of pointers to the boxes.
        std::vector<const ConfigBox*> filament_ptrs;
        for (const FilamentSettings& fs : m_filament_settings)
            filament_ptrs.push_back(&fs);
        std::vector<const ConfigBox*> tool_print_ptrs;
        for (const ToolPrintSettings& tps : m_tool_print_settings)
            tool_print_ptrs.push_back(&tps);

        ASSERT(filament_ptrs.size() == tool_print_ptrs.size());

        // Finally pass this to the base class. It only gets pointers to ConfigBoxes
        // and does not have to know which ones are there.
        add(&m_printer_settings);
        add(filament_ptrs);
        add(&m_print_settings);
        add(tool_print_ptrs);
    }

private:
    PrinterSettings m_printer_settings;
    std::vector<FilamentSettings> m_filament_settings;
    PrintSettings m_print_settings;
    std::vector<ToolPrintSettings> m_tool_print_settings;
};
