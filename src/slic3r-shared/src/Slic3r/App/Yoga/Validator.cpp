///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Validator.hpp"

#include <Slic3r/Assert.hpp>

#include <fmt/format.h>
#include <cmath>

namespace Slic3r::App::Yoga {

Validator::~Validator() {}

std::string Validator::process(const std::string& input) { return input; }

std::string IntValidator::process(const std::string& input)
{
    double value = 0;
    try {
        value = boost::get<double>(m_eval.eval(m_parser.parse(input)));
    } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
    } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
    }

    return std::to_string(std::clamp(static_cast<int>(std::round(value)), m_from, m_to));
}

IntValidator::IntValidator(int from, int to) : m_from(from), m_to(to) { ASSERT(from <= to); }

int IntValidator::from() const { return m_from; }

void IntValidator::set_from(int from) { m_from = from; }

int IntValidator::to() const { return m_to; }

void IntValidator::set_to(int to) { m_to = to; }

DoubleValidator::DoubleValidator(double from, double to) : m_from(from), m_to(to)
{
    ASSERT(from <= to);
}

double DoubleValidator::from() const { return m_from; }

void DoubleValidator::set_from(double from) { m_from = from; }

double DoubleValidator::to() const { return m_to; }

void DoubleValidator::set_to(double to) { m_to = to; }

std::string DoubleValidator::process(const std::string& input)
{
    double value = 0;
    try {
        value = boost::get<double>(m_eval.eval(m_parser.parse(input)));
    } catch ([[maybe_unused]] const Biz::Expr::ParseError& error) {
    } catch ([[maybe_unused]] const Biz::Expr::EvalError& error) {
    }

    return fmt::format("{}", std::clamp(value, m_from, m_to));
}

} // namespace Slic3r::App::Yoga
