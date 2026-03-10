///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/Expr/Parser.hpp"
#include "Slic3r/Biz/Expr/Eval.hpp"

#include <limits>
#include <string>

namespace Slic3r::App::Yoga {

class Validator
{
public:
    virtual ~Validator();

    virtual std::string process(const std::string& input);
};

class IntValidator : public Validator
{
public:
    IntValidator(int from = std::numeric_limits<int>::min(), int to = std::numeric_limits<int>::max());

    std::string process(const std::string& input) override;

    int from() const;
    void set_from(int from);

    int to() const;
    void set_to(int to);

    int value() const;

private:
    Slic3r::Biz::Expr::Parser m_parser;
    Slic3r::Biz::Expr::Eval m_eval;

    int m_value = 0; // holds parsed value
    int m_from  = 0;
    int m_to    = 0;
};

class DoubleValidator : public Validator
{
public:
    DoubleValidator(
        double from = std::numeric_limits<double>::lowest(),
        double to   = std::numeric_limits<double>::max()
    );

    std::string process(const std::string& input) override;

    double from() const;
    void set_from(double from);

    double to() const;
    void set_to(double to);

    double value() const;

    std::optional<int> precision() const;
    void set_precision(int precision);

private:
    Slic3r::Biz::Expr::Parser m_parser;
    Slic3r::Biz::Expr::Eval m_eval;

    double m_value = 0; // holds parsed value
    double m_from  = 0;
    double m_to    = 0;
    std::optional<int> m_precision;
};

class PercentageValidator : public DoubleValidator
{
public:
    PercentageValidator(
        double from = std::numeric_limits<double>::lowest(),
        double to   = std::numeric_limits<double>::max()
    );

    std::string process(const std::string& input) override;

    /**
     * @brief Indicates whether the last processed input contained a '%' symbol.
     * @return True if the last input contained a percentage symbol.
     */
    bool entered_percentage_symbol() const;

    /**
     * @brief Controls visibility of the '%' symbol in the displayed value.
     * @param visible If true, the percentage symbol will be visible.
     */
    void set_visible_percentage_symbol(bool visible);

private:
    bool m_last_entered_percentage_symbol = false;
    bool m_is_visible_percentage_symbol = false;
};

} // namespace Slic3r::App::Yoga
