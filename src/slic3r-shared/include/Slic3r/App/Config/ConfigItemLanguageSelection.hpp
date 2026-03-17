#pragma once

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigItemLanguageSelection : public ConfigItemControl, public Yoga::ComboBox
{
public:
    ConfigItemLanguageSelection(
        size_t index,
        const Domain::ConfigItem& config_item,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

protected:
    void on_data_update() override;

private:
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index = 0;
};

} // namespace Slic3r::App
