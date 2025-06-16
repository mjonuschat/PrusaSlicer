#pragma once

#include <nlohmann/json.hpp>
#include "Slic3r/Domain/TemplateUtils.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Percentage.hpp"

namespace Slic3r::Domain::Advanced {
[[maybe_unused]] void from_json(const nlohmann::ordered_json& json_value, Vec2d& vec2d);

[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const Vec2d& vec2d);

} // namespace Slic3r::Domain::Advanced

namespace Slic3r::Domain {
[[maybe_unused]] void from_json(const nlohmann::ordered_json& json_value, Percentage& percentage);

[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const Percentage& percentage);

[[maybe_unused]] void from_json(const nlohmann::ordered_json& json_value, FloatOrPercentage& fop);

[[maybe_unused]] void to_json(nlohmann::ordered_json& json_value, const FloatOrPercentage& fop);
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Config {

template <typename T>
bool is_valid(const nlohmann::ordered_json& json_value) = delete;

template<>
bool is_valid<bool>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<int>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<double>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<std::string>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<Domain::Vec2d>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<Domain::FloatOrPercentage>(const nlohmann::ordered_json& json_value);

template<>
bool is_valid<Domain::Percentage>(const nlohmann::ordered_json& json_value);

template <typename T>
bool is_valid_optional(const nlohmann::ordered_json& json_value);

template <typename T>
bool is_valid_vec(const nlohmann::ordered_json& json_value) {
    if (!json_value.is_array()) {
        return false;
    }

    for (const auto& element : json_value) {
        if constexpr (Domain::is_std_optional_v<typename T::value_type>) {
            if (!is_valid_optional<typename T::value_type>(element)) {
                return false;
            }
        } else {
            if (!is_valid<typename T::value_type>(element)) {
                return false;
            }
        }
    }
    return true;
}

template <typename T>
bool is_valid_optional(const nlohmann::ordered_json& json_value) {
    if (json_value.is_null()) {
        return true;
    }

    if constexpr (Domain::is_std_vector_v<typename T::value_type>) {
        return is_valid_vec<typename T::value_type>(json_value);
    } else {
        return is_valid<typename T::value_type>(json_value);
    }
}

} // namespace Slic3r::Biz::Config
