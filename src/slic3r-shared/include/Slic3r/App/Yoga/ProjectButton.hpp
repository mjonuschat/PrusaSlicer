#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {
    
class Text;
class LayoutButton;
class ProjectButtonBackground;

class ProjectButton : public AbstractButton {
public:
    ProjectButton(const std::string& name, size_t project_id);
    std::function<void()>& on_cross();

    size_t project_id() const;
    bool is_cross_hovered() const;

    bool is_selected();
    void set_selected(bool selected);

protected:
    void hovered_updated_internal() override;

private:
    ProjectButtonBackground* m_background{ nullptr };
    Text* m_label{ nullptr };
    LayoutButton* m_cross{ nullptr };
   
    size_t m_project_id{0};
    bool m_selected{ false };
};

} // namespace Slic3r::App::Yoga