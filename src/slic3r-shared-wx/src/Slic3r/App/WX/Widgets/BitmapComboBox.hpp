///|/ Copyright (c) Prusa Research 2021 - 2022 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "ComboBox.hpp"

// ---------------------------------
// ***  BitmapComboBox  ***
// ---------------------------------
namespace Slic3r::App::WX {

// BitmapComboBox used to presets list on Sidebar and Tabs
//class BitmapComboBox : public wxBitmapComboBox
class BitmapComboBox : public ComboBox
{
public:
BitmapComboBox( wxWindow*       parent,
                wxWindowID      id      = wxID_ANY,
                const wxString& value   = wxEmptyString,
                const wxPoint&  pos     = wxDefaultPosition,
                const wxSize&   size    = wxDefaultSize,
                int             n       = 0,
                const wxString  choices[] = NULL,
                long            tyle    = 0);
};

}
