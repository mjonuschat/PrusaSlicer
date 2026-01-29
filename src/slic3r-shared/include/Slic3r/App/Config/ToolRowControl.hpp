///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/ConfigDef.hpp>

#include "Slic3r/Biz/DataObserver.hpp"

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Domain {
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Icon;
class Text;
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl;
class ConfigItemPreview;

/**
 * @todo replace this back to Biz::PrintToolItem
 */
struct ToolRowOverride
{
    const Domain::ConfigItem* override_item{nullptr};
    const Domain::ConfigItem* print_item{nullptr};
    bool overriden{false};
    bool extruder_candidate{false};
};

using ToolRowOverridePtr = std::unique_ptr<ToolRowOverride>;

class ToolRowControl : public Biz::DataObserver<ToolRowOverride>, public Yoga::Item
{
public:
    explicit ToolRowControl(
        size_t index,
        const ToolRowOverride& data,
        Biz::IConfigBoxSetter& cb_setter
    );

protected:
    void on_data_update() override;

    void on_index_update() override;

private:
    Biz::IConfigBoxSetter& m_cb_setter;

    Domain::ConfigItemDef::GUIType m_control_gui_type{Domain::ConfigItemDef::GUIType::undefined};
    bool m_last_overriden = false;

    Yoga::Icon* m_icon{nullptr};
    Yoga::Text* m_label{nullptr};
    ConfigItemControl* m_control{nullptr};
    ConfigItemPreview* m_preview{nullptr};
    Yoga::Item* m_input{nullptr};
    Yoga::LayoutButton* m_switch_override{nullptr};
    Yoga::Text* m_default_label{nullptr};
    Yoga::Text* m_in_use_label{nullptr};
};

} // namespace Slic3r::App
