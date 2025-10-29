///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/ComboBoxListViewSelection.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::App {

class PrinterNozzleRow : public Biz::DataObserver<Biz::Preset::ToolConfigItemObservableList>, public Yoga::Item
{
public:
    explicit PrinterNozzleRow(
        size_t index,
        const Biz::Preset::ToolConfigItemObservableList& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

    void on_data_update() override;
    void on_index_update() override;
    void on_will_be_removed() override;

private:
    using ComboBoxTools = Yoga::ComboBoxListViewSelection<Domain::Preset::HwToolConfigDef>;

    Biz::Preset::PresetInteractor& m_preset_interactor;
    Yoga::Text* m_text_index   = nullptr;
    ComboBoxTools* m_combo_box = nullptr;
};

} // namespace Slic3r::App
