#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/Yoga/LayerHeightProfileControl.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
class Rectangle;
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {

class HeightRangeRow : public Yoga::AbstractButton
{
public:
    struct Callbacks
    {
        std::function<void()> delete_clicked = []() {};
        std::function<void()> undo_clicked   = []() {};
        std::function<void()> selected       = []() {};
        std::function<void(bool)> hovered    = [](bool) {};
    };

    explicit HeightRangeRow(const Yoga::HeightRangeEntry& height_range);

    Callbacks& callbacks();

    const Yoga::HeightRangeEntry& height_range() const;

    void set_height_range(const Yoga::HeightRangeEntry& height_range);
    void set_has_overrides(bool has_overrides);
    void set_selected(bool selected);
    void set_highlighted(bool highlighted);

protected:
    void hovered_updated_internal() override;

private:
    Yoga::HeightRangeEntry m_height_range;
    bool m_has_overrides{false};
    bool m_selected{false};
    bool m_highlighted{false};

    Yoga::Rectangle* m_background{nullptr};
    Yoga::Text* m_range_label{nullptr};
    Yoga::Text* m_height_label{nullptr};
    Yoga::LayoutButton* m_undo_button{nullptr};
    Yoga::LayoutButton* m_delete_button{nullptr};

    Callbacks m_callbacks;

    void update_background();
    void update_labels();
    void update_undo_button();
};

} // namespace Slic3r::App::Plater
