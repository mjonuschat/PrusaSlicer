///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/Config.hpp>
#include <Slic3r/Biz/IObservableList.hpp>

namespace Slic3r::Biz {

class ConfigBoxObservableList : public IObservableList<Domain::ConfigItem>
{
public:
    void set_config_box(Domain::ConfigBox* config_box);

    const Domain::ConfigItem& at(size_t index) const override;

    size_t size() const override;

    void set_value(const std::string_view key, const Domain::ConfigValue& value);

    const Domain::ConfigValue* find(const std::string& name) const;

private:
    Domain::ConfigBox* m_config_box{nullptr};
};

class ConfigBoxOverridesObservableList : public IObservableList<Domain::ConfigItem>
{
public:
    void set_config_box(Domain::ConfigBox* config_box);

    const Domain::ConfigItem& at(size_t index) const override;

    size_t size() const override;

    void set_value(const std::string& key, const Domain::ConfigValue& value);

    void set_override(const std::string& key, bool enable);

private:
    Domain::ConfigBox* m_config_box{nullptr};
};

} // namespace Slic3r::Biz
