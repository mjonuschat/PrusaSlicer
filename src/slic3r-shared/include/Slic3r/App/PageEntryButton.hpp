///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

namespace Slic3r::App {

struct PageEntry
{
    std::string name;
    Render::Icon icon = Render::Icon::None;
};

class PageEntryButton : public Yoga::LayoutButton, public Biz::DataObserver<PageEntry>
{
public:
    using FnIndexClicked = std::function<void(size_t)>;

    PageEntryButton(size_t index, const PageEntry& page_entry, FnIndexClicked on_clicked);

protected:
    void on_data_update() override;

private:
    FnIndexClicked m_on_clicked;
};

} // namespace Slic3r::App
