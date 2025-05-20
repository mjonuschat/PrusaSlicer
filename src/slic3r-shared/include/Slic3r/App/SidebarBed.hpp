#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include <vector>

namespace Slic3r::App {

namespace Yoga {
class Text;
class PrinterSettingsButton;
class MaterialSettingsButton;
} // namespace Yoga

class SidebarBed : public Yoga::Window
{
public:
    explicit SidebarBed();

private:
    Yoga::Text* m_bed_name{nullptr};
    Yoga::PrinterSettingsButton* m_printer{nullptr};
    std::vector<Yoga::MaterialSettingsButton*> m_filaments;
};

} // namespace Slic3r::App
