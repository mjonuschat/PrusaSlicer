#include "Slic3r/Domain/Units.hpp"
#include "Slic3r/App/I18N/I18N.hpp"

#include "Slic3r/Assert.hpp"

#include <vector>
#include <functional>
#include <cmath>

namespace Slic3r::Domain {

static const std::vector<std::string> UNITS_STR = {
    _u8L("bytes"),
    _u8L("KB"),
    _u8L("MB"),
    _u8L("GB"),
    _u8L("TB"),
    _u8L("\u00B0C"), // _u8L(u8"\u00B0C"),
    _u8L("d"),
    _u8L("\u00B0F"), // _u8L(u8"\u00B0F"),
    _u8L("ft"),
    _u8L("ft\u00B3"), // _u8L(u8"ft\u00B3")
    _u8L("g"),
    _u8L("h"),
    _u8L("in"),
    _u8L("in/s"),
    _u8L("in\u00B3"), // _u8L(u8"in\u00B3")
    _u8L("in\u00B3/s"), // _u8L(u8"in\u00B3/s")
    _u8L("m"),
    _u8L("m\u00B3"), // _u8L(u8"m\u00B3")
    _u8L("mm"),
    _u8L("mm/s"),
    _u8L("mm/m"),
    _u8L("mm\u00B3"), // _u8L(u8"mm\u00B3")
    _u8L("mm\u00B3/s"), // _u8L(u8"mm\u00B3/s")
    _u8L("m"),
    _u8L("oz"),
    _u8L("s"),
    _u8L("y"),
};

struct Conversion
{
    UnitsType in;
    UnitsType out;
    std::function<float(float)> conversion_function;
};

static const float INCHES_TO_MILLIMETERS = 25.4f;
static const float MILLIMETERS_TO_INCHES = 1.0f / INCHES_TO_MILLIMETERS;
static const float METERS_TO_FEET = 3.28084f;
static const float FEET_TO_METERS = 1.0f / METERS_TO_FEET;
static const float METERS_TO_MILLIMETERS = 1000.0f;
static const float MILLIMETERS_TO_METERS = 1.0f / METERS_TO_MILLIMETERS;
static const float FEET_TO_INCHES = 12.0f;
static const float INCHES_TO_FEET = 1.0f / FEET_TO_INCHES;
static const float CUBED_INCHES_TO_CUBED_MILLIMETERS = INCHES_TO_MILLIMETERS * INCHES_TO_MILLIMETERS * INCHES_TO_MILLIMETERS;
static const float CUBED_MILLIMETERS_TO_CUBED_INCHES = MILLIMETERS_TO_INCHES * MILLIMETERS_TO_INCHES * MILLIMETERS_TO_INCHES;
static const float CUBED_METERS_TO_CUBED_FEET = METERS_TO_FEET * METERS_TO_FEET * METERS_TO_FEET;
static const float CUBED_FEET_TO_CUBED_METERS = FEET_TO_METERS * FEET_TO_METERS * FEET_TO_METERS;
static const float CUBED_METERS_TO_CUBED_MILLIMETERS = METERS_TO_MILLIMETERS * METERS_TO_MILLIMETERS * METERS_TO_MILLIMETERS;
static const float CUBED_MILLIMETERS_TO_CUBED_METERS = MILLIMETERS_TO_METERS * MILLIMETERS_TO_METERS * MILLIMETERS_TO_METERS;
static const float CUBED_FEET_TO_CUBED_INCHES = FEET_TO_INCHES * FEET_TO_INCHES * FEET_TO_INCHES;
static const float CUBED_INCHES_TO_CUBED_FEET = INCHES_TO_FEET * INCHES_TO_FEET * INCHES_TO_FEET;
static const float GRAMS_TO_OUNCES = 0.035274f;
static const float OUNCES_TO_GRAMS = 1.0f / GRAMS_TO_OUNCES;
static const float MINUTES_TO_SECONDS = 60.0f;
static const float SECONDS_TO_MINUTES = 1.0f / MINUTES_TO_SECONDS;

static const std::vector<Conversion> CONVERSIONS = {
    { UnitsType::Celsius,                  UnitsType::Farhenheit,               [](float value){ return (value * 1.8f) + 32.0f; } },
    { UnitsType::Farhenheit,               UnitsType::Celsius,                  [](float value){ return (value - 32.0f) * 0.55555f; } },
    { UnitsType::Feet,                     UnitsType::Inches,                   [](float value){ return value * FEET_TO_INCHES; } },
    { UnitsType::Feet,                     UnitsType::Meters,                   [](float value){ return value * FEET_TO_METERS; } },
    { UnitsType::Grams,                    UnitsType::Ounces,                   [](float value){ return value * GRAMS_TO_OUNCES; } },
    { UnitsType::Inches,                   UnitsType::Feet,                     [](float value){ return value * INCHES_TO_FEET; } },
    { UnitsType::Inches,                   UnitsType::Millimeters,              [](float value){ return value * INCHES_TO_MILLIMETERS; } },
    { UnitsType::InchesPerSecond,          UnitsType::MillimetersPerSecond,     [](float value){ return value * INCHES_TO_MILLIMETERS; } },
    { UnitsType::InchesCube,               UnitsType::MillimetersCube,          [](float value){ return value * CUBED_INCHES_TO_CUBED_MILLIMETERS; } },
    { UnitsType::InchesCubePerSecond,      UnitsType::MillimetersCubePerSecond, [](float value){ return value * CUBED_INCHES_TO_CUBED_MILLIMETERS; } },
    { UnitsType::Meters,                   UnitsType::Millimeters,              [](float value){ return value * METERS_TO_MILLIMETERS; } },
    { UnitsType::Meters,                   UnitsType::Feet,                     [](float value){ return value * METERS_TO_FEET; } },
    { UnitsType::MetersCube,               UnitsType::MillimetersCube,          [](float value){ return value * CUBED_METERS_TO_CUBED_MILLIMETERS; } },
    { UnitsType::Millimeters,              UnitsType::Inches,                   [](float value){ return value * MILLIMETERS_TO_INCHES; } },
    { UnitsType::MillimetersPerSecond,     UnitsType::InchesPerSecond,          [](float value){ return value * MILLIMETERS_TO_INCHES; } },
    { UnitsType::MillimetersPerSecond,     UnitsType::MillimetersPerMinute,     [](float value){ return value * MINUTES_TO_SECONDS; } },
    { UnitsType::MillimetersPerMinute,     UnitsType::MillimetersPerSecond,     [](float value){ return value * SECONDS_TO_MINUTES; } },
    { UnitsType::MillimetersCube,          UnitsType::InchesCube,               [](float value){ return value * CUBED_MILLIMETERS_TO_CUBED_INCHES; } },
    { UnitsType::MillimetersCubePerSecond, UnitsType::InchesCubePerSecond,      [](float value){ return value * CUBED_MILLIMETERS_TO_CUBED_INCHES; } },
    { UnitsType::Millimeters,              UnitsType::Meters,                   [](float value){ return value * MILLIMETERS_TO_METERS; } },
    { UnitsType::MillimetersCube,          UnitsType::MetersCube,               [](float value){ return value * CUBED_MILLIMETERS_TO_CUBED_METERS; } },
    { UnitsType::Ounces,                   UnitsType::Grams,                    [](float value){ return value * OUNCES_TO_GRAMS; } },
    { UnitsType::Minutes,                  UnitsType::Seconds,                  [](float value){ return value * MINUTES_TO_SECONDS; } },
    { UnitsType::Seconds,                  UnitsType::Minutes,                  [](float value){ return value * SECONDS_TO_MINUTES; } },
};

float convert(float value, UnitsType value_units, UnitsType desired_units)
{
    DEBUG_ASSERT(value_units < UnitsType::COUNT && desired_units < UnitsType::COUNT);
    const auto it = std::find_if(CONVERSIONS.begin(), CONVERSIONS.end(), 
      [value_units, desired_units](const Conversion& c) { return c.in == value_units && c.out == desired_units; });
    DEBUG_ASSERT(value_units == desired_units || it != CONVERSIONS.end());
    return (it != CONVERSIONS.end()) ? it->conversion_function(value) : value;
}

std::string format_to_string(float value, UnitsType units, uint8_t decimals)
{
    DEBUG_ASSERT(units < UnitsType::COUNT);
    char buf[64];
    sprintf(buf, "%.*f %s", decimals, value, units_as_string(units).c_str());
    return std::string(buf);
}

std::string convert_and_format_to_string(float value, UnitsType value_units, UnitsType desired_units, uint8_t decimals,
    bool append_units)
{
    DEBUG_ASSERT(value_units < UnitsType::COUNT && desired_units < UnitsType::COUNT);
    const auto it = std::find_if(CONVERSIONS.begin(), CONVERSIONS.end(),
      [value_units, desired_units](const Conversion& c) { return c.in == value_units && c.out == desired_units; });

    if (value_units != desired_units && it != CONVERSIONS.end()) {
        // TODO -> log error message;
    }

    // if unable to perform the conversion, return the input value
    const float out_value = (it != CONVERSIONS.end()) ? convert(value, value_units, desired_units) : value;
    if (append_units)
        return format_to_string(out_value, (it != CONVERSIONS.end()) ? desired_units : value_units, decimals);
    else {
        char buf[64];
        sprintf(buf, "%.*f", decimals, out_value);
        return std::string(buf);
    }
}

std::string units_as_string(UnitsType units)
{
    DEBUG_ASSERT(units < UnitsType::COUNT);
    return UNITS_STR[std::size_t(units)];
}

struct TimeHDMS
{
    float days{ 0.0f };
    float hours{ 0.0f };
    float minutes{ 0.0f };
    float seconds{ 0.0f };

    std::string format() const {
        char buffer[64];
        if (days > 365.0f)
            return "> 1" + units_as_string(UnitsType::Years);
        else if (days > 0.0f)
            sprintf(buffer, d_h_m_s_mask().c_str(), int(days), int(hours), int(minutes), int(seconds));
        else if (hours > 0.0f)
            sprintf(buffer, h_m_s_mask().c_str(), int(hours), int(minutes), int(seconds));
        else if (minutes > 0.0f)
            sprintf(buffer, m_s_mask().c_str(), int(minutes), int(seconds));
        else if (seconds >= 1.0f)
            sprintf(buffer, s_mask().c_str(), int(std::round(seconds)));
        else
            return "< 1" + units_as_string(UnitsType::Seconds);
        return buffer;
    }

    std::string format_short() const {
        TimeHDMS copy = *this;
        if (copy.seconds >= 30.0f) {
            copy.minutes += 1.0f;
            if (copy.minutes == 60.0f) {
                copy.minutes = 0.0f;
                copy.hours += 1.0f;
                if (copy.hours == 24) {
                    copy.hours = 0.0f;
                    copy.days += 1.0f;
                }
            }
        }

        char buffer[64];
        if (copy.days > 365.0f)
            return "> 1" + units_as_string(UnitsType::Years);
        else if (copy.days > 0.0f)
            sprintf(buffer, d_h_m_mask().c_str(), int(copy.days), int(copy.hours), int(copy.minutes));
        else if (copy.hours > 0.0f)
            sprintf(buffer, h_m_mask().c_str(), int(copy.hours), int(copy.minutes));
        else if (copy.minutes > 0.0f)
            sprintf(buffer, m_mask().c_str(), int(copy.minutes));
        else if (copy.seconds >= 1.0f)
            sprintf(buffer, s_mask().c_str(), int(std::round(copy.seconds)));
        else
            return "< 1" + units_as_string(UnitsType::Seconds);
        return buffer;
    }

    std::string format_short_and_splitted() const {
        char buffer[64];
        if (days > 365.0f)
            return "> 1" + units_as_string(UnitsType::Years);
        else if (days > 0.0f)
            sprintf(buffer, dh__m_mask().c_str(), int(days), int(hours), int(minutes));
        else if (hours > 0.0f) {
            if (hours < 10.0f && minutes < 10.0f && seconds < 10.0f)
                sprintf(buffer, hms_mask().c_str(), int(hours), int(minutes), int(seconds));
            else if (hours > 10.0f && minutes > 10.0f && seconds > 10.0f)
                sprintf(buffer, h__m__s_mask().c_str(), int(hours), int(minutes), int(seconds));
            else if ((minutes < 10.0f && seconds > 10.0f) || (minutes > 10.0f && seconds < 10.0f))
                sprintf(buffer, h__ms_mask().c_str(), int(hours), int(minutes), int(seconds));
            else
                sprintf(buffer, hm__s_mask().c_str(), int(hours), int(minutes), int(seconds));
        }
        else if (minutes > 0.0f) {
            if (minutes > 10.0f && seconds > 10.0f)
                sprintf(buffer, m__s_mask().c_str(), int(minutes), int(seconds));
            else
                sprintf(buffer, ms_mask().c_str(), int(minutes), int(seconds));
        }
        else if (seconds >= 1.0f)
            sprintf(buffer, s_mask().c_str(), int(std::round(seconds)));
        else
            return "< 1" + units_as_string(UnitsType::Seconds);
        return buffer;
    }

    static std::string days_mask()    { return "%d" + units_as_string(UnitsType::Days); }
    static std::string hours_mask()   { return "%d" + units_as_string(UnitsType::Hours); }
    static std::string minutes_mask() { return "%d" + units_as_string(UnitsType::Minutes); }
    static std::string seconds_mask() { return "%d" + units_as_string(UnitsType::Seconds); }

    static std::string d_h_m_s_mask() { return days_mask() + " " + hours_mask() + " " + minutes_mask() + " " + seconds_mask(); }
    static std::string d_h_m_mask()   { return days_mask() + " " + hours_mask() + " %d" + minutes_mask(); }
    static std::string h_m_s_mask()   { return hours_mask() + " " + minutes_mask() + " " + seconds_mask(); }
    static std::string h_m_mask()     { return hours_mask() + " " + minutes_mask(); }
    static std::string m_s_mask()     { return minutes_mask() + " " + seconds_mask(); }
    static std::string m_mask()       { return minutes_mask(); }
    static std::string s_mask()       { return seconds_mask(); }
    static std::string hms_mask()     { return hours_mask() + minutes_mask() + seconds_mask(); }
    static std::string ms_mask()      { return minutes_mask() + seconds_mask(); }
    static std::string h__m__s_mask() { return hours_mask() + "\n" + minutes_mask() + "\n" + seconds_mask(); }
    static std::string dh__m_mask()   { return days_mask() + hours_mask() + "\n" + minutes_mask(); }
    static std::string hm__s_mask()   { return hours_mask() + minutes_mask() + "\n" + seconds_mask(); }
    static std::string h__ms_mask()   { return hours_mask() + "\n" + minutes_mask() + seconds_mask(); }
    static std::string m__s_mask()    { return minutes_mask() + "\n" + seconds_mask(); }
};

static TimeHDMS time_dhms(float time_in_secs)
{
    TimeHDMS ret;
    ret.days = float(int(time_in_secs / 86400.0f));
    time_in_secs -= ret.days * 86400.0f;
    ret.hours = float(int(time_in_secs / 3600.0f));
    time_in_secs -= ret.hours * 3600.0f;
    ret.minutes = float(int(time_in_secs / 60.0f));
    ret.seconds = time_in_secs - ret.minutes * 60.0f;
    return ret;
}

std::string format_time_dhms(float time_in_secs)
{
    return time_dhms(time_in_secs).format();
}

std::string format_time_dhms_short(float time_in_secs)
{
    return time_dhms(time_in_secs).format_short();
}

std::string format_time_dhms_short_and_splitted(float time_in_secs)
{
    return time_dhms(time_in_secs).format_short_and_splitted();
}

} // namespace Slic3r::Domain
