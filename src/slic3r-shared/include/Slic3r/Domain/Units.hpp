#pragma once

#include <string>
#include <cstddef>

namespace Slic3r::Domain {

enum class UnitsType : uint8_t
{
    Bytes,
    KiloBytes,
    MegaBytes,
    GigaBytes,
    TeraBytes,
    Celsius,
    Days,
    Farhenheit,
    Feet,
    FeetCube,
    Grams,
    Hours,
    Inches,
    InchesPerSecond,
    InchesCube,
    InchesCubePerSecond,
    Meters,
    MetersCube,
    Millimeters,
    MillimetersPerSecond,
    MillimetersPerMinute,
    MillimetersCube,
    MillimetersCubePerSecond,
    Minutes,
    Ounces,
    Seconds,
    Years,
    COUNT
};

static constexpr size_t UNITS_TYPES_COUNT = size_t(UnitsType::COUNT);

enum class UnitsSystem : uint8_t
{
    SI,
    Imperial,
    COUNT
};

static constexpr size_t UNITS_SYSTEMS_COUNT = size_t(UnitsSystem::COUNT);

/** @brief Convert the given value from value_units to desired_units.
 *
 * @param value The value to convert.
 * @param value_units The initial units.
 * @param desired_units The final units.
 */
float convert(float value, UnitsType value_units, UnitsType desired_units);

/** @brief Format the given value into a string with units.
 *
 * @param value The value to format.
 * @param units The units.
 * @param decimals The number of decimals to add after the point.
 */
std::string format_to_string(float value, UnitsType units, uint8_t decimals = 0);

/** @brief Convert the given value from value_units to desired_units.
 *         and format the result into a string with units.
 *
 * @param value The value to format.
 * @param value_units The initial units.
 * @param desired_units The final units.
 * @param decimals The number of decimals to add after the point.
 * @param append_units Whether or not to append the units string.
 */
std::string convert_and_format_to_string(float value, UnitsType value_units, UnitsType desired_units, uint8_t decimals = 0,
    bool append_units = true);

/** @brief Return the string associated with the given units.
 *
 * @param units The units.
 */
std::string units_as_string(UnitsType units);

/** @brief Format the given time in seconds into a string with format
 *         days, hours, minutes, seconds.
 *
 * @param time_in_secs The time to format, in seconds.
 */
std::string format_time_dhms(float time_in_secs);

/** @brief Format the given time in seconds into a string with format
 *         days, hours, minutes, seconds, rounding the result to
 *         get a shorter string.
 *
 * @param time_in_secs The time to format, in seconds.
 */
std::string format_time_dhms_short(float time_in_secs);

/** @brief Format the given time in seconds into a string with format
 *         days, hours, minutes, seconds, rounding the result to
 *         get a shorter string and splitting it in multiple lines, if needed.
 *
 * @param time_in_secs The time to format, in seconds.
 */
std::string format_time_dhms_short_and_splitted(float time_in_secs);

} // namespace Slic3r::Domain
