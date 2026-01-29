///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ToolLabel : public Biz::DataObserver<bool>, public Yoga::LayoutButton
{
public:
    explicit ToolLabel(size_t index, const bool& data);

protected:
    void on_index_update() override;
    void on_data_update() override;

    void update_markings();

private:
    Yoga::Text* m_label{nullptr};
};

} // namespace Slic3r::App
