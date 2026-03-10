///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Validator.hpp"

#include <Slic3r/Assert.hpp>

#include <fmt/format.h>
#include <cmath>

namespace {
std::string_view trim(std::string_view string)
{
    auto begin = std::find_if_not(string.begin(), string.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });

    auto end = std::find_if_not(string.rbegin(), string.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();

    if (begin >= end) {
        return {}; // empty view
    }
    return std::string_view(&*begin, static_cast<size_t>(end - begin));
}
} // namespace

namespace Slic3r::App::Yoga {

Validator::~Validator() {}

std::string Validator::process(const std::string& input)
{
    return input;
}

std::string IntValidator::process(const std::string& input)
{
    double value = 0;
    try {
        value = boost::get<double>(m_eval.eval(m_parser.parse(input)));
    } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
    } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
    }

    m_value = std::clamp(static_cast<int>(std::round(value)), m_from, m_to);

    return std::to_string(m_value);
}

int IntValidator::value() const
{
    return m_value;
}

IntValidator::IntValidator(int from, int to) : m_from(from), m_to(to)
{
    ASSERT(from <= to);
}

int IntValidator::from() const
{
    return m_from;
}

void IntValidator::set_from(int from)
{
    m_from = from;
}

int IntValidator::to() const
{
    return m_to;
}

void IntValidator::set_to(int to)
{
    m_to = to;
}

DoubleValidator::DoubleValidator(double from, double to) : m_from(from), m_to(to)
{
    ASSERT(from <= to);
}

double DoubleValidator::from() const
{
    return m_from;
}

void DoubleValidator::set_from(double from)
{
    m_from = from;
}

double DoubleValidator::to() const
{
    return m_to;
}

void DoubleValidator::set_to(double to)
{
    m_to = to;
}

double DoubleValidator::value() const
{
    return m_value;
}

std::string DoubleValidator::process(const std::string& input)
{
    double value = 0;
    try {
        value = boost::get<double>(m_eval.eval(m_parser.parse(input)));
    } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
    } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
    }

    m_value = std::clamp(value, m_from, m_to);
    return m_precision.has_value() ? fmt::format("{1:.{0}f}", m_precision.value(), m_value) :
                                     fmt::format("{:.10g}", m_value);
}

std::optional<int> DoubleValidator::precision() const
{
    return m_precision;
}

void DoubleValidator::set_precision(int precision)
{
    m_precision = precision;
}

PercentageValidator::PercentageValidator(double from, double to) : DoubleValidator(from, to) {}

std::string PercentageValidator::process(const std::string& input)
{
    std::string_view trimmed_view = trim(input);
    if (!trimmed_view.empty() && trimmed_view.back() == '%') {
        m_last_entered_percentage_symbol = true;
        trimmed_view.remove_suffix(1); // so it would confuse parser
    } else {
        m_last_entered_percentage_symbol = false;
    }

    const std::string trimmed_string{trimmed_view};
    const std::string processed_string = DoubleValidator::process(trimmed_string);
    return (m_last_entered_percentage_symbol && m_is_visible_percentage_symbol) ?
        fmt::format("{}%", processed_string) :
        fmt::format("{}", processed_string);
}

bool PercentageValidator::entered_percentage_symbol() const
{
    return m_last_entered_percentage_symbol;
}

void PercentageValidator::set_visible_percentage_symbol(bool visible)
{
    m_is_visible_percentage_symbol = visible;
}

} // namespace Slic3r::App::Yoga
