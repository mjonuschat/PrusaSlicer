///|/ Copyright (c) Prusa Research 2021 - 2022 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/WX/Widgets/BitmapComboBox.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"

// ---------------------------------
// ***  BitmapComboBox  ***
// ---------------------------------

namespace Slic3r::App::WX::Widgets {

BitmapComboBox::BitmapComboBox(wxWindow*        parent,
                               wxWindowID       id          /* = wxID_ANY*/,
                               const wxString&  value       /* = wxEmptyString*/,
                               const wxPoint&   pos         /* = wxDefaultPosition*/,
                               const wxSize&    size        /* = wxDefaultSize*/,
                               int              n           /* = 0*/,
                               const wxString   choices[]   /* = NULL*/,
                               long             style       /* = 0*/) :
    ComboBox(parent, id, value, pos, size, n, choices, style | DD_NO_CHECK_ICON)
{
    SetFont(w_config()->normal_font());
}

}

