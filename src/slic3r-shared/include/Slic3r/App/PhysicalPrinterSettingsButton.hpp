///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include "Slic3r/Biz/PhysicalPrinter/PhysicalPrinterConfig.hpp"

#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"

namespace Slic3r::App::Yoga {
class Icon;
class LayoutButton;
}

namespace Slic3r::App {

class PhysicalPrinterSettingsButton
    : public Yoga::PrinterSettingsButton,
    public Biz::DataObserver<Biz::PhysicalPrinter::PhysicalPrinterConfig>
{
public:
    using FnIndexClicked = std::function<void(size_t)>;

    PhysicalPrinterSettingsButton(
        size_t index,
        const Biz::PhysicalPrinter::PhysicalPrinterConfig& physical_printer,
        FnIndexClicked on_clicked,
        FnIndexClicked on_cog_clicked,
        FnIndexClicked on_bin_clicked
    );

    void update_button_text();

    void set_icon(Render::Icon icon);

    void set_visible_bin(bool is_visible);
    std::function<void()>& on_bin();

    void update();

    std::string uuid() const;

    void set_compatible(bool compatible);

protected:
    void on_data_update() override;

    void update_btns_visibility() override;

private:
    FnIndexClicked m_on_clicked;
    FnIndexClicked m_on_cog_clicked;
    FnIndexClicked m_on_bin_clicked;

    Yoga::LayoutButton*  m_bin_btn{nullptr};
    bool                 m_is_visible_bin{ false };

    Yoga::Icon*          m_attention_icon{nullptr};
};

} // namespace Slic3r::App
