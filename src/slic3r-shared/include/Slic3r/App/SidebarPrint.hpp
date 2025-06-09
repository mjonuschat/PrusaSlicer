#pragma once
#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App {

namespace Yoga {
class LayoutButton;
class InputTextField;
} // namespace Yoga

class SidebarPrint : public Yoga::Window
{
public:
    SidebarPrint();

private:
    void add_separator();
    void add_row(const std::string& label, std::unique_ptr<Yoga::Item> control);

private:
    Yoga::LayoutButton* m_settings_set_btn{nullptr};
    Yoga::InputTextField* m_input_text_perimeters{nullptr};
    Yoga::Item* m_rows{nullptr};
};

} // namespace Slic3r::App
