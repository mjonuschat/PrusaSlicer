#pragma once

#include "libslic3r/CustomGCode.hpp"
#include <Slic3r/App/libvgcode/Types.hpp>

#include <functional>
#include <set>

namespace Slic3r::App::LibvgcodeWrapper {

struct ExtrudersSequence;

enum class PrinterTechnology : uint8_t
{
    Unknown,
    FFF,
    SLA,
    Any,
    COUNT
};

static constexpr size_t PRINTER_TECHNOLOGIES_COUNT = size_t(PrinterTechnology::COUNT);

typedef std::function<void(void)>                                   InvalidateSliceCallback;
typedef std::function<void(void)>                                   TicksChangedCallback;
typedef std::function<void(const Slic3r::CustomGCode::Info&)>       UpdateLayersSlider;
typedef std::function<std::vector<std::string>(void)>               GetExtruderColorsCallback;
typedef std::function<bool(void)>                                   AutoColorChangeCallback;
typedef std::function<void(Slic3r::CustomGCode::Type)>              CheckGCodeCallback;
typedef std::function<bool(ExtrudersSequence&)>                     GetExtrudersSequenceCallback;
typedef std::function<std::string(const std::string&, float)>       GetCustomGCodeCallback;
typedef std::function<std::string(const std::string&, float)>       GetPausePrintMsgCallback;
typedef std::function<std::string(const std::string&)>              GetNewColorCallback;
typedef std::function<int(const std::string&, int)>                 ShowInfoMsgCallback;
typedef std::function<std::string(Slic3r::CustomGCode::Type)>       GetGCodeCallback;
typedef std::function<std::set<int>(float)>                         GetUsedExtrudersInPrintCallback;
typedef std::function<void(void)>                                   GCodeViewTypeChangedCallback;
typedef std::function<void(void)>                                   ExtrusionRoleVisibilityChangedCallback;
typedef std::function<void(const std::string&, const std::string&)> AppConfigChangedCallback;

std::string to_string(Biz::libpgcode::MoveType type);
std::string to_string(Domain::GCodeExtrusionRole role);
std::string to_string(Biz::libpgcode::OptionType type);
std::string to_string(libvgcode::ViewType type, Biz::libpgcode::UnitsSystem sys);

} // namespace Slic3r::App::LibvgcodeWrapper
