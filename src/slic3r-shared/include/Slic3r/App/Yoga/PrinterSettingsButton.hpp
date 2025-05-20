#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"

namespace Slic3r::App::Yoga {

class PrinterSettingsButton : public LayoutButton
{
public:
    explicit PrinterSettingsButton(const std::string& tooltip = {});

    void set_printer_name(const std::string& printer_name);
    void set_preset_name(const std::string& preset_name);
    void set_printing_state(int state);

    void set_visible_printer(bool is_visible);
    std::function<void()>& on_printer();

    void set_visible_cog(bool is_visible);
    std::function<void()>& on_cog();

protected:
    void checked_updated_internal() override;
    void hovered_updated_internal() override;
    void update_btns_visibility();

private:
    Text*           m_printer_name{nullptr};
    Text*           m_preset_name{nullptr};
    LayoutButton*   m_cog_btn{nullptr};
    LayoutButton*   m_printers_btn{nullptr};

    bool            m_is_visible_printers{ false };
    bool            m_is_visible_cog{ false };
};

} // namespace Slic3r::App::Yoga
