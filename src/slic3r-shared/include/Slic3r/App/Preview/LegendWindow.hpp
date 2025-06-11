#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Preview/Legend.hpp"

namespace Slic3r::App::Yoga {
class Rectangle;
class ToggleButton;
class ComboBox;
}

namespace Slic3r::App::Preview {

class LegendWindow : public Yoga::Window
{
public:
    LegendWindow(libvgcode::FdmViewer* viewer, FdmViewerWrapper* wrapper);
    LegendCallbacks& callbacks();
    void update_type_selector(const std::vector<std::string>& types, int selection);

private:
    void update_show_time_estimate(const libvgcode::FdmViewer& viewer);

private:
    Legend* m_legend{ nullptr };
    Yoga::ComboBox* m_type_selector{ nullptr };

    Yoga::ToggleButton* m_show_time_estimate{ nullptr };
    Yoga::ToggleButton* m_detail_view{ nullptr };

    Yoga::Rectangle* m_settings{ nullptr };
};

} // namespace Slic3r::App::Preview