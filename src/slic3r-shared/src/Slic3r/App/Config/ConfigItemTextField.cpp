///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemTextField.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include <imgui_internal.h>
#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemTextField::ConfigItemTextField(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    if (data.def().multiline) {
        set_flags(flags() | ImGuiInputTextFlags_Multiline);
        set_height(100);
    }

    if (*m_state->def().type == typeid(double)
        || *m_state->def().type == typeid(Domain::Percentage)
        || *m_state->def().type == typeid(Domain::FloatOrPercentage))
    {
        m_validator = std::make_unique<DoubleValidator>(
            m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
            m_state->def().max.value_or(std::numeric_limits<double>::max())
        );

        set_validator(m_validator.release());
    }

    set_min_size({150, 0});
    set_tooltip(data.def().tooltip);
    m_tooltip.set_text_wrap(true);
    m_tooltip.content_item()->set_width(350);

    on_data_update();

    callbacks().text_edited = [this]() {
        if (*m_state->def().type == typeid(std::string)) {
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{text()});
        } else if (*m_state->def().type == typeid(double)) {
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{m_validator->value()});
        } else if (*m_state->def().type == typeid(Domain::Percentage)) {
            m_preset_interactor.set_item_value(
                *m_state,
                Domain::ConfigValue{Domain::Percentage{m_validator->value()}}
            );
        } else if (*m_state->def().type == typeid(Domain::FloatOrPercentage)) {
            const std::string value_text = text();
            if (value_text.find('%') != std::string::npos) {
                m_preset_interactor.set_item_value(
                    *m_state,
                    Domain::ConfigValue{
                        Domain::FloatOrPercentage{Domain::Percentage{m_validator->value()}}
                    }
                );
            } else {
                m_preset_interactor.set_item_value(
                    *m_state,
                    Domain::ConfigValue{Domain::FloatOrPercentage{m_validator->value()}}
                );
            }
        }
    };
}

void ConfigItemTextField::on_data_update()
{
    if (*m_state->def().type == typeid(std::string)) {
        set_text(m_state->value().get<std::string>());
    } else if (*m_state->def().type == typeid(double)) {
        set_text(fmt::format("{}", m_state->value().get<double>()));
    } else if (*m_state->def().type == typeid(Domain::Percentage)) {
        set_text(fmt::format("{} %", m_state->value().get<Domain::Percentage>().value));
    } else if (*m_state->def().type == typeid(Domain::FloatOrPercentage)) {
        Domain::FloatOrPercentage value = m_state->value().get<Domain::FloatOrPercentage>();
        set_text(
            value.is_percentage() ? fmt::format("{} %", value.percentage().value) :
                                    fmt::format("{}", value.float_value())
        );
    }
    // set_text(m_state->value().get())
}

} // namespace Slic3r::App
