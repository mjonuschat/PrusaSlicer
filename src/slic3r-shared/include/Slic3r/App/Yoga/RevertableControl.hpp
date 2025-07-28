///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

namespace Slic3r::App::Yoga {

class LayoutButton;

class RevertableControl
{
public:
    virtual ~RevertableControl();

    void set_revert_button(LayoutButton* button);
    LayoutButton* revert_button() const;

protected:
    virtual bool is_changed_value() const;

    virtual void reset() {}

    void update_revert_button();

private:
    LayoutButton* m_revert_button{nullptr};
};

} // namespace Slic3r::App::Yoga
