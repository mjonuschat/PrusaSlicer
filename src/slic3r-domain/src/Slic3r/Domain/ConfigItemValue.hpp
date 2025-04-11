#pragma once

#include <vector>

class ConfigItemValue
{
public:
    virtual ~ConfigItemValue() = default;
    virtual ConfigItemValue* clone() = 0;

    void set_null(bool null) { is_null = null; }
    bool get_null() const { return is_null; }

protected:
    ConfigItemValue() = default;

    bool is_null = false;
};


template <class T>
class ConfigItemValueSingle : public ConfigItemValue
{
public:
    ConfigItemValueSingle<T>* clone() {
        auto ptr = new ConfigItemValueSingle<T>();
        ptr->set(value); ptr->is_null = is_null;
        return ptr;
    }
    void set(const T& value_) { value = value_; }
    T& get() { ASSERT(! is_null); return value; }

protected:
    T value{};
};



template <class T>
class ConfigItemValueVector : public ConfigItemValue
{
public:
    ConfigItemValueVector<T>* clone() {
        auto ptr = new ConfigItemValueVector<T>();
        ptr->get() = values;
        ptr->is_null = is_null;
        return ptr;
    }
    std::vector<T>& get() { ASSERT(! is_null); return values; }

private:
    std::vector<T> values;
};





using ConfigItemValueBool    = ConfigItemValueSingle<bool>;
using ConfigItemValueInt     = ConfigItemValueSingle<int>;
using ConfigItemValueDouble  = ConfigItemValueSingle<double>;
using ConfigItemValueString  = ConfigItemValueSingle<std::string>;
using ConfigItemValueBools   = ConfigItemValueVector<bool>;
using ConfigItemValueInts    = ConfigItemValueVector<int>;
using ConfigItemValueDoubles = ConfigItemValueVector<double>;
using ConfigItemValueStrings = ConfigItemValueVector<std::string>;





class ConfigItemValueFloatOrPercent : public ConfigItemValueDouble
{
public:
    // Inherits get/set and nullability + respective data members.

    ConfigItemValueFloatOrPercent* clone() {
        auto ptr = new ConfigItemValueFloatOrPercent();
        ptr->set(value);
        ptr->set_percent(m_is_percent);
        ptr->is_null = is_null; 
        return ptr;
    }

    // Adds the differentiation between number and percent.
    void set_percent(bool is_percent) { m_is_percent = is_percent; }
    bool is_percent() const { return m_is_percent; }

private:
    bool m_is_percent{false};
};



class ConfigItemValueEnum : public ConfigItemValueInt
{
public:
    ConfigItemValueEnum* clone() {
        auto ptr = new ConfigItemValueEnum();
        ptr->set(value);
        ptr->is_null = is_null;
        return ptr;
    }
};



class ConfigItemValuePercent : public ConfigItemValueDouble
{
public:
    ConfigItemValuePercent* clone() {
        auto ptr = new ConfigItemValuePercent();
        ptr->set(value);
        ptr->is_null = is_null;
        return ptr;
    }
};