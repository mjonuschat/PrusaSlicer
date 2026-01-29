///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/DataObserver.hpp"

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/IConfigNavigable.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Text;
class ToggleButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl;
class ConfigItemSpinBox;

class ConfigRowItem :
    public Biz::DataObserver<Domain::ConfigItem>,
    public Yoga::Rectangle,
    public IConfigNavigable
{
public:
    ConfigRowItem(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cb_setter,
        size_t cbi_index,
        bool small,
        std::optional<std::string> force_label = std::nullopt
    );

    void navigate_to_item(const Domain::ConfigItem* config_item) override;
    void clear_navigation() override;

    void set_label_text_color(const ImColor& color);

private:
    void on_data_update() override;

private:
    Biz::IConfigBoxSetter& m_cb_setter;
    bool m_small{false};
    size_t m_cbi_index{0};
    std::optional<std::string> m_force_label;

    Domain::ConfigItemDef::GUIType m_created_gui_type{Domain::ConfigItemDef::GUIType::undefined};
    const std::type_info* m_created_value_type{nullptr};
    Yoga::Item* m_left_side{nullptr};
    Yoga::Text* m_label{nullptr};
    Yoga::Text* m_sidetext{nullptr};
    Yoga::Item* m_input{nullptr};
    Yoga::ToggleButton* m_toggle_enable{nullptr};

    ConfigItemControl* m_control{nullptr};
    ConfigItemSpinBox* m_config_item_spin_box{
        nullptr
    }; ///< valid only if ConfigItem gui type is spinbox
};

} // namespace Slic3r::App
