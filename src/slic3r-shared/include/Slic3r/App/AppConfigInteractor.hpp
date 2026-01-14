///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

#include "Slic3r/App/IAppConfigChangedListener.hpp"

namespace Slic3r::App {

/**
 * Manipulates config box associated with application configuration
 */
class AppConfigInteractor final :
    public WithListeners<IAppConfigChangedListener>,
    public Biz::IConfigBoxSetter
{
public:
    explicit AppConfigInteractor(Domain::ConfigBox* app_config_box);

    AppConfigInteractor(AppConfigInteractor&&) = default;

    Biz::ConfigBoxInteractor& app_config_cbi();

    const Domain::ConfigValue*
    get_override_original_value(const Domain::ConfigItem& item, size_t index = 0) const override;

    void set_item_value(
        const std::string& item_name,
        const Domain::ConfigValue& value,
        size_t index = 0
    );

    void set_item_value(
        const Domain::ConfigItem& item,
        const Domain::ConfigValue& value,
        size_t index = 0
    ) override;

    void set_item_override(const Domain::ConfigItem& item, bool enable, size_t index = 0) override {
    };

private:
    Biz::ConfigBoxInteractor::SetAccessor m_cbi_accessor;
    Biz::ConfigBoxInteractor m_app_config_cbi;
};

} // namespace Slic3r::App
