///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/ComboBoxListView.hpp"
#include "Slic3r/Biz/ObservableListWithSelection.hpp"

namespace Slic3r::App::Yoga {

template <class Data>
class ComboBoxListViewSelection :
    public ComboBoxListView<Data>,
    public Biz::IListSelectionChangedListener
{
public:
    void on_list_selection_changed(Domain::SelectionId new_selection) override
    {
        ComboBox::set_current_index(static_cast<int>(new_selection));
    }
};

} // namespace Slic3r::App::Yoga
