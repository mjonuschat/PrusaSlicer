#include "Slic3r/Domain/ConfigValue.hpp"

namespace Slic3r::Domain {

void EnumVectorWrapper::set_strings(const std::vector<std::string>& values) {
    std::vector<int> payload(values.size());

    size_t i = 0;
    bool ok = false;
    for (const std::string& value : values) {
        for (const EnumValueDef& evd : *m_def) {
            if (evd.str_serialized == value) {
                payload[i] = evd.enum_value;
                ok = true;
                break;
            }
        }
        ASSERT(ok);
        ++i;
    }

    m_values = payload;
}

const std::type_info* EnumVectorWrapper::type() const {
    return m_type;
}

const std::vector<int> EnumVectorWrapper::values() const {
    return m_values;
}

const EnumValueDefs& EnumVectorWrapper::def() const {
    return *m_def;
}

std::vector<std::string_view> EnumVectorWrapper::get_strings() const {
    std::vector<std::string_view> out;
    for (int value : m_values) {
        auto it = std::ranges::find_if(*m_def,
            [value](const Domain::EnumValueDef& evd) { return evd.enum_value == value; });
        ASSERT(it != m_def->end());
        out.push_back(it->str_serialized);
    }
    return out;
}

bool EnumVectorWrapper::operator==(const EnumVectorWrapper&) const = default;

void EnumWrapper::set_string(const std::string& value)
{
    for (const EnumValueDef& evd : *m_def) {
        if (evd.str_serialized == value) {
            m_value = evd.enum_value;
            return;
        }
    }

    // Maybe should be an exception, as serialized value can be user input.
    PANIC("Failed to set enum from string!");
}

const std::type_info* EnumWrapper::type() const { return m_type; }

const int EnumWrapper::value() const { return m_value; }

const EnumValueDefs& EnumWrapper::def() const {
    return *m_def;
};

std::string_view EnumWrapper::get_string() const {
    for (const EnumValueDef& evd : *m_def)
        if (evd.enum_value == m_value)
            return evd.str_serialized;
    PANIC("Failed to get enum as string!");
}

bool EnumWrapper::operator==(const EnumWrapper&) const = default;

ConfigValue::ConfigValue(const char* str): ConfigValue(std::string{str}) {}

ConfigValue::ConfigValue(const ConfigValue& other): m_value{other.m_value} {
    assert_types_equal(other);
}

ConfigValue::ConfigValue(ConfigValue&& other) noexcept: m_value{std::move(other.m_value)} {
    assert_types_equal(other);
}

ConfigValue& ConfigValue::operator=(const ConfigValue& other) {
    assert_types_equal(other);
    m_value = other.m_value;
    return *this;
}

ConfigValue& ConfigValue::operator=(ConfigValue&& other) noexcept {
    assert_types_equal(other);
    m_value = std::move(other.m_value);
    return *this;
}

ConfigValue::~ConfigValue() = default;

bool ConfigValue::operator==(const ConfigValue& rhs) const = default;

void ConfigValue::assert_types_equal(const ConfigValue& other) const {
    return;
    ASSERT(m_value.index() == other.m_value.index(), "Only a value of the same type can be assigned to ConfigItem!");
    if (std::holds_alternative<EnumWrapper>(m_value)) {
        ASSERT(
            std::get<EnumWrapper>(m_value).type() == std::get<EnumWrapper>(other.m_value).type(),
            "Enum types must be the same!"
        );
    }
    if (std::holds_alternative<EnumVectorWrapper>(m_value)) {
        ASSERT(
            std::get<EnumVectorWrapper>(m_value).type() == std::get<EnumVectorWrapper>(other.m_value).type(),
            "Types of enums in vector must be the same!"
        );
    }
}
}
