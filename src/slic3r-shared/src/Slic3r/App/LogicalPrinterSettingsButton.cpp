///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/LogicalPrinterSettingsButton.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Log.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

LogicalPrinterSettingsButton::LogicalPrinterSettingsButton(
    size_t index,
    const Biz::Preset::PresetItem& logical_printer,
    FnIndexClicked on_clicked,
    FnIndexClicked on_cog_clicked,
    const Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Biz::Preset::PresetItem>(index, logical_printer),
    m_on_clicked(on_clicked),
    m_on_cog_clicked(on_cog_clicked),
    m_preset_interactor(preset_interactor)
{
    on_data_update();
    set_flex_shrink(0);
    set_checkable(false);
    set_visible_cog(true);

    callbacks().action = [this]() { m_on_clicked(m_index); };
    on_cog() = [this]() { m_on_cog_clicked(m_index); };
}

void LogicalPrinterSettingsButton::on_data_update()
{
    const std::string prefix{m_state->runtime_only ? Biz::_u8L("(From 3mf) ") : ""};
    set_printer_name(prefix + m_state->name);
    set_preset_name(m_state->hw_printer_config_name);

    const Domain::Preset::HwPrinterConfig& printer_config =
        m_preset_interactor.get_printer_config(m_state->hw_printer_config_id).first.get();

    if (printer_config.visual.thumbnail.has_value()) {
        const std::string image_path =
            printer_config.relative_path_to_assets() + printer_config.visual.thumbnail.value();
        set_image(image_path);
    }
}

} // namespace Slic3r::App