#include "Slic3r/Biz/Parser/IO.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "LocalesUtils.hpp"

#include <map>
#include <string>
#include <vector>

namespace Slic3r::Biz::Parser::IO {

using Domain::Percentage;
using Domain::FloatOrPercentage;
using Domain::Vec2d;

// clang-format off
template<typename T>
struct get_option_type { static constexpr auto value{Type::None}; };
template<> struct get_option_type<double> { static constexpr auto value{Type::Float}; };
template<> struct get_option_type<std::vector<double>> { static constexpr auto value{Type::Floats}; };
template<> struct get_option_type<int> { static constexpr auto value{Type::Int}; };
template<> struct get_option_type<std::vector<int>> { static constexpr auto value{Type::Ints}; };
template<> struct get_option_type<std::optional<int>> { static constexpr auto value{Type::IntOptional}; };
template<> struct get_option_type<std::vector<std::optional<int>>> { static constexpr auto value{Type::IntOptionals}; };
template<> struct get_option_type<std::string> { static constexpr auto value{Type::String}; };
template<> struct get_option_type<std::vector<std::string>> { static constexpr auto value{Type::Strings}; };
template<> struct get_option_type<Percentage> { static constexpr auto value{Type::Percent}; };
template<> struct get_option_type<std::vector<Percentage>> { static constexpr auto value{Type::Percents}; };
template<> struct get_option_type<FloatOrPercentage> { static constexpr auto value{Type::FloatOrPercent}; };
template<> struct get_option_type<std::vector<FloatOrPercentage>> { static constexpr auto value{Type::FloatsOrPercents}; };
template<> struct get_option_type<Vec2d> { static constexpr auto value{Type::Point}; };
template<> struct get_option_type<std::vector<Vec2d>> { static constexpr auto value{Type::Points}; };
template<> struct get_option_type<bool> { static constexpr auto value{Type::Bool}; };
template<> struct get_option_type<std::vector<bool>> { static constexpr auto value{Type::Bools}; };
// clang-format on

template<typename T>
constexpr Type get_option_type_v = get_option_type<T>::value;

template<typename T>
requires (!std::is_enum_v<T>)
Scalar::Scalar(const T& value, const std::string& ratio_over) : m_ratio_over{ratio_over}
{
    constexpr auto type{get_option_type_v<T>};
    static_assert(type != Type::None, "Unknown type passed to set");

    if (type != Type::Percent && type != Type::FloatOrPercent) {
        ASSERT(ratio_over.empty(), "Ration over can only be defined for percent values");
    }

    m_value = value;
    m_type = type;
}
template Scalar::Scalar(const double&, const std::string&);
template Scalar::Scalar(const int&, const std::string&);
template Scalar::Scalar(const std::optional<int>&, const std::string&);
template Scalar::Scalar(const std::string&, const std::string&);
template Scalar::Scalar(const Percentage&, const std::string&);
template Scalar::Scalar(const FloatOrPercentage&, const std::string&);
template Scalar::Scalar(const Vec2d&, const std::string&);
template Scalar::Scalar(const bool&, const std::string&);

bool Scalar::operator==(const Scalar& rhs) const {
    if (m_type != rhs.m_type) {
        return false;
    }
    switch (m_type) {
        case Type::None: return true;
        case Type::Float: return get<double>() == rhs.get<double>();
        case Type::Int: return get<int>() == rhs.get<int>();
        case Type::String: return get<std::string>() == rhs.get<std::string>();
        case Type::Percent: return get<Percentage>() == rhs.get<Percentage>();
        case Type::FloatOrPercent: return get<FloatOrPercentage>() == rhs.get<FloatOrPercentage>();
        case Type::Point: return get<Vec2d>().isApprox(rhs.get<Vec2d>());
        case Type::Bool: return get<bool>() == rhs.get<bool>();
        case Type::Enum: return get<int>() == rhs.get<int>();
        default: PANIC("Programming error: invalid type reached!");
    }
}

Type Scalar::type() const { return m_type; }

// Escape \n, \r and backslash
std::string escape_string_cstyle(const std::string &str)
{
    // Allocate a buffer twice the input string length,
    // so the output will fit even if all input characters get escaped.
    std::vector<char> out(str.size() * 2, 0);
    char *outptr = out.data();
    for (size_t i = 0; i < str.size(); ++ i) {
        char c = str[i];
        if (c == '\r') {
            (*outptr ++) = '\\';
            (*outptr ++) = 'r';
        } else if (c == '\n') {
            (*outptr ++) = '\\';
            (*outptr ++) = 'n';
        } else if (c == '\\') {
            (*outptr ++) = '\\';
            (*outptr ++) = '\\';
        } else
            (*outptr ++) = c;
    }
    return std::string(out.data(), outptr - out.data());
}

std::string Scalar::serialize() const {
    switch(m_type){
    case Type::Float: return float_to_string_decimal_point(get<double>());
    case Type::Int: return std::to_string(get<int>());
    case Type::IntOptional: {
        const auto result{get<std::optional<int>>()};
        return result ? std::to_string(*result) : "nil";
    }
    case Type::String: return escape_string_cstyle(get<std::string>());
    case Type::Percent: {
        const auto result{get<Percentage>()};
        return float_to_string_decimal_point(result.value) + "%";
    }
    case Type::FloatOrPercent: {
        const auto result{get<FloatOrPercentage>()};
        const std::string value_string{float_to_string_decimal_point(result.get_abs_value(100.0))};
        return result.is_percentage() ? value_string + "%" : value_string;
    }
    case Type::Point: {
        const auto result{get<Vec2d>()};
        return float_to_string_decimal_point(result.x()) + "," +
            float_to_string_decimal_point(result.y());
    }
    case Type::Bool: {
        const auto result{get<bool>()};
        return result ? "1" : "0";
    }
    case Type::Enum: return *ASSERT_VAL(m_serialized_value);
    default: PANIC("Programming error: invalid type reached!");
    }
}

std::string Scalar::ratio_over() const { return m_ratio_over; }

template<typename T>
requires (!std::is_enum_v<T>)
T Scalar::get() const
{
    ASSERT(get_option_type_v<T> == m_type);
    const T* result{std::any_cast<T>(&m_value)};
    ASSERT(result != nullptr);
    return *result;
}
template double Scalar::get() const;
template int Scalar::get() const;
template std::optional<int> Scalar::get() const;
template std::string Scalar::get() const;
template Percentage Scalar::get() const;
template FloatOrPercentage Scalar::get() const;
template Vec2d Scalar::get() const;
template bool Scalar::get() const;

template<typename T>
requires (!is_pair_v<T>)
Vector::Vector(const std::vector<T>& values)
{
    m_values.clear();
    constexpr auto type{std::is_enum_v<T> ? Type::Enums : get_option_type_v<std::vector<T>>};
    static_assert(type != Type::None, "Unknown type passed to set");
    m_type = type;
    for (const T& value : values) {
        m_values.push_back(Scalar{value});
    }
};
template Vector::Vector(const std::vector<double>&);
template Vector::Vector(const std::vector<int>&);
template Vector::Vector(const std::vector<std::optional<int>>&);
template Vector::Vector(const std::vector<std::string>&);
template Vector::Vector(const std::vector<Percentage>&);
template Vector::Vector(const std::vector<FloatOrPercentage>&);
template Vector::Vector(const std::vector<Vec2d>&);
template Vector::Vector(const std::vector<bool>&);

bool Vector::operator==(const Vector& rhs) const {
    if (m_type != rhs.m_type) {
        return false;
    }
    return m_values == rhs.m_values;
}

Type Vector::type() const { return m_type; }
std::size_t Vector::size() const { return m_values.size(); }
bool Vector::empty() const { return m_values.empty(); }

template<typename T>
requires (!std::is_enum_v<T>)
std::vector<T> Vector::get() const
{
    std::vector<T> result;
    for (const Scalar& value : m_values) {
        result.push_back(value.get<T>());
    }
    return result;
}
template std::vector<double> Vector::get() const;
template std::vector<int> Vector::get() const;
template std::vector<std::optional<int>> Vector::get() const;
template std::vector<std::string> Vector::get() const;
template std::vector<Percentage> Vector::get() const;
template std::vector<FloatOrPercentage> Vector::get() const;
template std::vector<Vec2d> Vector::get() const;
template std::vector<bool> Vector::get() const;

Scalar& Vector::at(const std::size_t index) { return m_values.at(index); }
const Scalar& Vector::at(const std::size_t index) const { return m_values.at(index); };

bool is_scalar(const Value& value) { return std::holds_alternative<Scalar>(value); }
bool is_vector(const Value& value) { return std::holds_alternative<Vector>(value); }

Type type(const Value& value)
{
    return std::visit([](auto&& value) { return value.type(); }, value);
};

template<typename T>
struct is_std_vector : std::false_type
{};

template<typename T, typename Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type
{};

template<typename T>
constexpr bool is_std_vector_v = is_std_vector<T>::value;

const Value* Config::option(const std::string& key) const
{
    const auto value_it{m_values.find(key)};
    if (value_it == m_values.end()) {
        return nullptr;
    }
    return &value_it->second;
}

Value* Config::optptr(const std::string& key) { return const_cast<Value*>(option(key)); }

template<typename T>
void Config::set(const std::string& key, const T& value)
{
    using InputType = typename std::remove_reference_t<T>;
    if constexpr (std::is_same_v<InputType, Value>) {
        m_values.insert_or_assign(key, value);
    } else {
        if constexpr (is_std_vector_v<T>) {
            m_values.insert_or_assign(key, Vector{value});
        } else {
            m_values.insert_or_assign(key, Scalar{value});
        }
    }
}
template void Config::set(const std::string&, const Value&);
template void Config::set(const std::string&, const double&);
template void Config::set(const std::string&, const int&);
template void Config::set(const std::string&, const std::optional<int>&);
template void Config::set(const std::string&, const std::string&);
template void Config::set(const std::string&, const Percentage&);
template void Config::set(const std::string&, const FloatOrPercentage&);
template void Config::set(const std::string&, const Vec2d&);
template void Config::set(const std::string&, const bool&);
template void Config::set(const std::string&, const std::vector<double>&);
template void Config::set(const std::string&, const std::vector<int>&);
template void Config::set(const std::string&, const std::vector<std::optional<int>>&);
template void Config::set(const std::string&, const std::vector<std::string>&);
template void Config::set(const std::string&, const std::vector<Percentage>&);
template void Config::set(const std::string&, const std::vector<FloatOrPercentage>&);
template void Config::set(const std::string&, const std::vector<Vec2d>&);
template void Config::set(const std::string&, const std::vector<bool>&);

void Config::apply(const Config& config) {
    for (const std::string &opt_key : config.keys()) {
        set(opt_key, *config.option(opt_key));
    }
}

std::vector<std::string> Config::keys() const {
    std::vector<std::string> keys;
    keys.reserve(m_values.size());
    std::ranges::transform(m_values, std::back_inserter(keys), [](const auto& pair) {
        return pair.first;
    });
    return keys;
};

} // namespace Slic3r::Parser::IO
