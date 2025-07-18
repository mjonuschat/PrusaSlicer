///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/App/Plater/Measure.hpp"
#include "Slic3r/App/Plater/MeasureGizmoHelper.hpp"

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

        void reset();
        void set_from(const Measure::FeatureItem& feature);

    private:
        Yoga::Text* m_name = nullptr;
        Yoga::Text* m_value = nullptr;
    };

    SpotDescription& spot1();
    SpotDescription& spot2();

    void set_measure(const Measure::MeasurementResult& result);
    void show_measure(bool show);

private:
    void add_measure_row(const std::string& name, const std::string& value, const std::string& units);
    void add_spot_row(const ImColor& marker, const std::string& title, Yoga::ItemPtr controls);

private:
    struct MeasureRowItem
    {
        Yoga::Text* name{nullptr};
        Yoga::Text* value{ nullptr };
        std::string clipboard_text;
        Yoga::LayoutButton* copy_btn{ nullptr };
    };
    std::vector<Yoga::Item*> m_measure_rows;
    std::vector<MeasureRowItem> m_measure_row_items;

    SpotDescription* m_spot1 = nullptr;
    SpotDescription* m_spot2 = nullptr;

    Yoga::Item* m_helper_panel = nullptr;
    Yoga::Item* m_main_panel = nullptr;
    Yoga::GizmoDialogHelp m_extra_help;

    Yoga::Item* m_measures_item{ nullptr };
};

} // namespace Slic3r::App::Plater
