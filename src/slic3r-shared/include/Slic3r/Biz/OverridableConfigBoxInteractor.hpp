///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/IListObserver.hpp"

#include <memory>

namespace Slic3r::Domain {
struct ConfigBox;
struct ConfigValue;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class OverridableConfigBoxObservableList;

class OverridableConfigBoxInteractor
{
public:
    class SetAccessor
    {
    public:
        void set_source(std::weak_ptr<OverridableConfigBoxObservableList> config_box_list);

        void set_value(const std::string& key, const Domain::ConfigValue& value);

        void set_override(const std::string& key, bool enable);

        void set_config_box(Domain::ConfigBox* config_box);

    private:
        std::weak_ptr<OverridableConfigBoxObservableList> m_config_box_list;
    };

    explicit OverridableConfigBoxInteractor(SetAccessor& set_accessor, Domain::ConfigBox* config_box);
    OverridableConfigBoxInteractor();

    const Domain::ConfigValue* find(const std::string& name) const;

    std::weak_ptr<const OverridableConfigBoxObservableList> config_box_overridable_list() const;

    std::weak_ptr<OverridableConfigBoxObservableList> config_box_overridable_list();

private:
    UnsharedPointer<OverridableConfigBoxObservableList> m_config_box_list;
};

} // namespace Slic3r::Biz
