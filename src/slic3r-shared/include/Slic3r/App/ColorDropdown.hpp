#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/ContextPopup.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/IColorsChangedListener.hpp"

namespace Slic3r::App::Yoga {

class MulticolorCircle;

class ColorMenuItem : public Yoga::RectangleButton
{
public:
    ColorMenuItem(
        const std::string& label,
        const std::vector<Domain::ColorRGBA>& colors,
        bool selectable,
        bool dropdown_indicator = false,
        bool hollow = false,
        std::optional<std::string> index = std::nullopt
    );

    void set_entry(
        const std::string& label,
        const std::vector<Domain::ColorRGBA>& colors,
        bool hollow = false,
        std::optional<std::string> index = std::nullopt
    );

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

private:
    MulticolorCircle* m_swatch{nullptr};
    Text* m_text{nullptr};
    std::string m_label;
    bool m_dropdown_indicator{false};
};

class ColorDropdown :
    public Item,
    public Biz::IColorsChangedListener,
    public Biz::Preset::IPresetChangedListener
{
public:
    ColorDropdown(Biz::ProjectInteractor& project_interactor, bool with_default, bool with_numbers);
    ~ColorDropdown();

    void set_current_index(std::optional<std::size_t> index);

    std::size_t current_index() const;

    void style_node() override;

    std::function<void(std::size_t index)> on_color_selected{[](std::size_t index) {}};

    void on_colors_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        const std::vector<Domain::ColorRGB>& colors
    ) override;

    void on_preset_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::PresetItemType type
    ) override;

protected:
    void reload();
private:
    void reload_default_colors();
    void rebuild_popup_items();
    std::string get_default_text();
    void update_trigger_label();
    void set_current_index_internal(std::size_t index);
    void set_items(const std::vector<std::pair<Domain::ColorRGBA, std::string>>& material_colors);

    Biz::ProjectInteractor& m_project_interactor;
    bool m_with_default{};
    bool m_with_numbers{};
    std::vector<std::pair<Domain::ColorRGBA, std::string>> m_material_colors;
    std::optional<std::size_t> m_current_index;
    ColorMenuItem* m_trigger{nullptr};
    ContextPopup* m_popup{nullptr};
    std::vector<ColorMenuItem*> m_popup_items;
    std::vector<Domain::ColorRGBA> m_default_colors;
};

} // namespace Slic3r::App::Yoga
