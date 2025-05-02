#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

class Rectangle;
class Icon;
class Text;

class LayoutButton : public AbstractButton {
public:
    LayoutButton(const std::string& label, Item* parent = nullptr);
    LayoutButton(const std::string& label, wchar_t icon, Item* parent = nullptr);

    const std::string& label() const;
    void set_label(const std::string& label);

    const ImColor& background_color() const;
    void set_background_color(const ImColor& color);

private:
    Rectangle* m_background = nullptr;
    Icon* m_icon = nullptr;
    Text* m_text = nullptr;
};

}
