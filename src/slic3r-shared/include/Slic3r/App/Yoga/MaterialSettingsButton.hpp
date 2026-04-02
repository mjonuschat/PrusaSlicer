#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Preset/IPresetChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"
#include "Slic3r/Biz/IColorsChangedListener.hpp"
#include "Slic3r/Biz/ProjectSettingsInteractor.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {
class MaterialSettingsDialog;
} // namespace Slic3r::App

namespace Slic3r::App::Yoga {

class Text;
class ColorPickerButton;
class ButtonGroup;

class MaterialSettingsButton :
    public RectangleButton,
    public Biz::DataObserver<Biz::Preset::PresetItemObservableList>,
    public Biz::IListSelectionChangedListener,
    public Biz::Preset::IPresetChangedListener,
    public Biz::IColorsChangedListener
{
public:
    MaterialSettingsButton(
        size_t index,
        const Biz::Preset::PresetItemObservableList& state,
        std::weak_ptr<ButtonGroup> button_group,
        Biz::ProjectInteractor& project_interactor
    );
    ~MaterialSettingsButton();

    void on_list_selection_changed(Domain::SelectionId new_selection) override;

    void on_hw_item_selection_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        Biz::Preset::HwItemType type
    ) override;

    void on_colors_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        const std::vector<Domain::ColorRGB>& colors
    ) override;

protected:
    void on_data_update() override;

    void set_color(const ImColor& color);
    void set_nozzle(const std::string& nozzle);
    void set_material_name(const std::string& name);

private:
    Biz::ListenerScope<
        Biz::Preset::IPresetChangedListener,
        Biz::Preset::PresetInteractor,
        MaterialSettingsButton>
        m_preset_changed_listener_scope;

    Biz::ListenerScope<
        Biz::IColorsChangedListener,
        Biz::ProjectSettingsInteractor,
        MaterialSettingsButton>
        m_colors_changed_listener_scope;

    ColorPickerButton* m_color_marker{nullptr};
    Text* m_material_name{nullptr};
    Text* m_nozzle{nullptr};
    std::weak_ptr<ButtonGroup> m_button_group;
    Biz::ProjectInteractor& m_project_interactor;
};

} // namespace Slic3r::App::Yoga
