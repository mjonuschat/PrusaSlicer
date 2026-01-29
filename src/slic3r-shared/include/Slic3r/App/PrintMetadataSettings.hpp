///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class PrintMetadataSettings :
    public Biz::DataObserver<Biz::Preset::ToolConfigItemObservableList>,
    public Yoga::Item
{
public:
    PrintMetadataSettings(
        size_t index,
        const Biz::Preset::ToolConfigItemObservableList& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;
    void on_index_update() override;

    void add_new_row(const std::string& label, Yoga::ItemPtr control);

    void update_contents();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;

    Yoga::InputTextField* m_input_id{nullptr};
    Yoga::InputTextField* m_input_name{nullptr};
    // Yoga::InputTextField* m_input_expression{nullptr};
};

} // namespace Slic3r::App
