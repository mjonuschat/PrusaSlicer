///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

#include <memory>

namespace Slic3rc::App::Yoga {
class Text;
class LayoutButton;
}

namespace Slic3r::App::Plater {
class MeasureDialog : public Yoga::GizmoDialog
{
public:
    MeasureDialog();

    enum class MeasureType {
        Angle,
        Distance,
    };

    struct SpotDescription : public Yoga::Item {
        SpotDescription();

        void set_as_plane();
        void set_as_edge(float lenth);

    private:
        Yoga::Text* m_name = nullptr;
        Yoga::Text* m_value = nullptr;
    };

    SpotDescription& spot1();
    SpotDescription& spot2();
    void set_measure(MeasureType type, float value);

    void show_measure(bool show);

    std::function<void()>& on_copy();

private:

    void add_measure_row();
    void add_spot_row(const ImColor& marker, const std::string& title, std::unique_ptr<Item> controls);

private:
    Yoga::Text* m_measure_name = nullptr;
    Yoga::Text* m_measure_value = nullptr;
    Yoga::LayoutButton* m_copy_btn = nullptr;

    SpotDescription* m_spot1 = nullptr;
    SpotDescription* m_spot2 = nullptr;

    Yoga::Item* m_helper_panel = nullptr;
    Yoga::Item* m_main_panel = nullptr;
};

} // namespace Slic3r::App::Plater
