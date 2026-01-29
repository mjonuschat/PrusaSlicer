///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/ConfigBoxObservableList.hpp"

namespace Slic3r::Biz {

/**
 * @brief The ConfigBoxInteractor class acts as Wrapper around ConfigBox
 * and provides IObservableList functionality.
 * @warning All set_value has to go through this Interactor, otherwise
 * there is a high chance of introducing desync.
 */
class ConfigBoxInteractor
{
public:
    class SetAccessor
    {
    public:
        void set_source(
            std::weak_ptr<ConfigBoxObservableList> config_box_list,
            std::weak_ptr<ConfigBoxOverridesObservableList> config_box_overrides_list
        );

        void set_value(const std::string& key, const Domain::ConfigValue& value);

        void set_override(const std::string& key, bool enable);

        void set_config_box(Domain::ConfigBox* config_box);

    private:
        std::weak_ptr<ConfigBoxObservableList> m_config_box_list;
        std::weak_ptr<ConfigBoxOverridesObservableList> m_config_box_overrides_list;
    };

    ConfigBoxInteractor();
    explicit ConfigBoxInteractor(SetAccessor& set_accessor, Domain::ConfigBox* config_box);

    const Domain::ConfigValue* find(const std::string& name) const;

    std::weak_ptr<ConfigBoxObservableList> config_box_list();

    std::weak_ptr<const ConfigBoxObservableList> config_box_list() const;

    std::weak_ptr<ConfigBoxOverridesObservableList> config_box_overrides_list();

private:
    // These shared_ptrs should not be shared to anywhere, only pass weak_ptrs
    // TODO: Implement a suitable container class for it.
    UnsharedPointer<ConfigBoxObservableList> m_config_box_list;
    UnsharedPointer<ConfigBoxOverridesObservableList> m_config_box_overrides_list;
};

} // namespace Slic3r::Biz
