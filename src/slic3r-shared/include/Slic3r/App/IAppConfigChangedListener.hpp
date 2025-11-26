///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

class IAppConfigChangedListener
{
public:
    virtual ~IAppConfigChangedListener() = default;

    virtual void on_app_config_changed() {}
};
